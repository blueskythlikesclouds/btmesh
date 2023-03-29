#pragma once

class bina
{
protected:
    FILE* file_;

    struct write_offset_command
    {
        int alignment;
        std::unique_ptr<std::function<void()>> function;
        size_t offset;
        size_t offset_position;
    };

    struct write_string_offset_command
    {
        std::string value;
        size_t offset;
        std::vector<size_t> offset_positions;
    };

    std::vector<write_offset_command> write_offset_commands_;
    std::vector<write_string_offset_command> write_string_offset_commands_;

    void execute_write_offset_commands(int first, int last);
    void begin_write() const;
    void end_write();

public:
    bina(FILE* file)
        : file_(file)
    {
        begin_write();
    }

    bina(const char* file_path)
        : bina(fopen(file_path, "wb"))
    {
    }

    ~bina()
    {
        close();
    }

    inline void write(void* memory, size_t size)
    {
        fwrite(memory, 1u, size, file_);
    }

    template<typename T>
    inline void write(const T& value) const
    {
        fwrite(&value, sizeof(T), 1u, file_);
    }

    template<typename T>
    inline void write_offset(int alignment, const T& function)
    {
        write_offset_command command;
        command.alignment = alignment;
        command.function = std::make_unique<std::function<void()>>(function);
        command.offset_position = ftell(file_);

        write_offset_commands_.push_back(std::move(command));

        write<size_t>(0);
    }

    void write_string_offset(const std::string& value);

    void align(int alignment) const;

    void close();
};