#include "bina.h"

void bina::execute_write_offset_commands(int first, int last)
{
    for (int i = first; i < last; i++)
    {
        auto& write_offset_command = write_offset_commands_[i];

        align(write_offset_command.alignment);
        write_offset_command.offset = ftell(file_);

        const int cur_first = write_offset_commands_.size();
        write_offset_command.function();
        const int cur_last = write_offset_commands_.size();

        execute_write_offset_commands(cur_first, cur_last);
    }
}

void bina::begin_write() const
{
    int memory[16];
    memset(memory, 0, sizeof(memory));
    fwrite(memory, sizeof(memory), 1u, file_);
}

void bina::end_write()
{
    execute_write_offset_commands(0, write_offset_commands_.size());

    align(4);
    const size_t data_end_position = ftell(file_);

    for (auto& command : write_string_offset_commands_)
    {
        command.offset = ftell(file_);
        fwrite(command.value.c_str(), 1u, command.value.size() + 1, file_);
    }

    align(4);
    const size_t strings_end_position = ftell(file_);

    std::set<size_t> offset_positions;

    for (auto& command : write_offset_commands_)
    {
        offset_positions.insert(command.offset_position);
    }

    for (auto& command : write_string_offset_commands_)
    {
        for (const size_t offset_position : command.offset_positions)
        {
            offset_positions.insert(offset_position);
        }
    }

    size_t cur_position = 64;

    for (const size_t offset_position : offset_positions)
    {
        const size_t distance = (offset_position - cur_position) >> 2;

        if (distance > 0x3FFF)
        {
            write<int>(_byteswap_ulong(0xC0000000 | distance));
        }
        else if (distance > 0x3F)
        {
            write<short>(_byteswap_ushort(0x8000 | distance));
        }
        else
        {
            write<char>(0x40 | distance);
        }

        cur_position = offset_position;
    }

    align(4);
    const size_t offset_positions_end_position = ftell(file_);

    fseek(file_, 0, SEEK_SET);

    write<size_t>(0x4C303132414E4942);
    write<int>(offset_positions_end_position);
    write<int>(1);
    write<int>(0x41544144);
    write<int>(offset_positions_end_position - 16);
    write<int>(data_end_position - 64);
    write<int>(strings_end_position - data_end_position);
    write<int>(offset_positions_end_position - strings_end_position);
    write<int>(24);

    for (auto& command : write_offset_commands_)
    {
        fseek(file_, command.offset_position, SEEK_SET);
        write<size_t>(command.offset - 64);
    }

    for (auto& command : write_string_offset_commands_)
    {
        for (const size_t offset_position : command.offset_positions)
        {
            fseek(file_, offset_position, SEEK_SET);
            write<size_t>(command.offset - 64);
        }
    }

    fseek(file_, 0, SEEK_END);
}

void bina::write_string_offset(const std::string& value)
{
    const size_t offset_position = ftell(file_);

    if (!value.empty())
    {
        bool found = false;

        for (auto& command : write_string_offset_commands_)
        {
            if (command.value == value)
            {
                command.offset_positions.push_back(offset_position);

                found = true;
                break;
            }
        }

        if (!found)
        {
            write_string_offset_command command;
            command.value = value;
            command.offset_positions.push_back(offset_position);

            write_string_offset_commands_.push_back(std::move(command));
        }
    }

    write<size_t>(0);
}

void bina::align(int alignment) const
{
    const size_t cur_position = ftell(file_);
    size_t padding_size = ((cur_position + alignment - 1) & ~(alignment - 1)) - cur_position;

    while (padding_size > 0)
    {
        write<char>(0);
        --padding_size;
    }
}
