# btmesh

btmesh is a tool that converts FBX files to .btmesh/.pccol files for use in Sonic Frontiers.

## Usage

To use btmesh, drag and drop an FBX file onto the executable. The output will be .btmesh/.pccol files in the same directory.

## Tags

You can assign material types and flags to collision shapes by appending tags to their names. Tags are case-insensitive. Here's an example:

```
CollisionMesh -> MyCollisionMesh@Grass
AlsoCollisionMesh -> AlsoCollisionMesh@Snow@NOT_STAND@slide
```

### Types

```
@NONE
@STONE
@EARTH
@WOOD
@GRASS
@IRON
@SAND
@LAVA
@GLASS
@SNOW
@NO_ENTRY
@ICE
@WATER
@SEA
@DAMAGE
@DEAD
@FLOWER0
@FLOWER1
@FLOWER2
@AIR
@DEADLEAVES
@WIREMESH
@DEAD_ANYDIR
@DAMAGE_THROUGH
@DRY_GRASS
@RELIC
@GIANT
@GRAVEL
@MUD_WATER
@SAND2
@SAND3
```

### Flags

```
@NOT_STAND
@BREAKABLE
@REST
@UNSUPPORTED
@REFLECT_LASER
@LOOP
@WALL
@SLIDE
@PARKOUR
@DECELERATE
@MOVABLE
@PRESS_DEAD
@RAYBLOCK
@WALLJUMP
@PUSH_BOX
@STRIDER_FLOOR
@GIANT_TOWER
@TEST_GRASS
@TEST_WATER
```