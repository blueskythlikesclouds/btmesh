# btmesh

btmesh is a tool that converts FBX files to .btmesh/.pccol files for use in Sonic Frontiers.

## Usage

To use btmesh, drag and drop an FBX file onto the executable. The output will be .btmesh/.pccol files in the same directory.

## Tags

Appending tags to the names of individual meshes allows you to assign material layers, types, and flags, as well as determine the shape of the mesh. Tags are case-insensitive. Here's an example:

```
CollisionMesh -> CollisionMesh@GRASS
WaterMesh -> WaterMesh@Water@Convex@Liquid
```

### Shapes

```
@CONVEX
```

### Layers

```
@NONE
@SOLID
@LIQUID
@THROUGH
@CAMERA
@SOLID_ONEWAY
@SOLID_THROUGH
@SOLID_TINY
@SOLID_DETAIL
@LEAF
@LAND
@RAYBLOCK
@EVENT
@RESERVED13
@RESERVED14
@PLAYER
@ENEMY
@ENEMY_BODY
@GIMMICK
@DYNAMICS
@RING
@CHARACTER_CONTROL
@PLAYER_ONLY
@DYNAMICS_THROUGH
@ENEMY_ONLY
@SENSOR_PLAYER
@SENSOR_RING
@SENSOR_GIMMICK
@SENSOR_LAND
@SENSOR_ALL
@RESERVED30
@RESERVED31
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
@PARKOUR_KNUCKLES
@PRESS_DEAD
@RAYBLOCK
@WALLJUMP
@PUSH_BOX
@STRIDER_FLOOR
@GIANT_TOWER
@PUSHOUT_LANDING
@TEST_GRASS
@TEST_WATER
```