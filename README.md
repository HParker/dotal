# Dotal

A small language designed to compile to the Uxn virtual machine.

This goal is to provide joyful way to write programs that has features tailored to the features of Uxn.

## Status

This is a pet/sideproject. Not everything will work! I don't expect the existing syntax to change, but I do expect that some changes around lexical scope and veriable handling and how the program is assembled will need to change to support larger projects and more interesting programming constructs. There are also still some things that are less expressive then I would like. There are a lot of magic numbers you need to know to write programs. You can see some of my thinking in the `todo.org` file.

I wrote this as a part of a bigger "visual compiler explorers" project that in ongoing. You can see some evidence of it in some of the artifacts that the compiler is able to export.

Example program:

```
sprite: square
11111111
10000001
10000001
10000001
10000001
10000001
10000001
11111111

fn main() do
  theme #202 #c1c #ece #905

  send(.Screen/x, 100)
  send(.Screen/y, 100)

  send(.Screen/addr, :sprite_square)
  send(.Screen/sprite, 67)
end
```

Simple program which draws a square to the screen.

## Install

1. clone the project
2. `make` (make sure you have relatively recent flex and a C compiler)

## Compile and Run a program

```
./lang examples/sprite.dt > test.tal
~/Downloads/uxn/uxnasm test.tal out.rom
~/Downloads/uxn/uxnemu out.rom
```

To learn more about the language, look at the examples in the `examples/` directory.

## Some notable syntax

### Hello World

A hello world program in dotal looks like

```
fn main() do
  put("hello world")
end
```

programs begin at the `main` function.

### Sprite literals

You can specify your sprite literals with a basic "ascii art" syntax

A simple mouse cursor might look like,

```
sprite: cursor
10000000
11000000
11100000
11110000
11111000
11111100
00000000
00000000
```

### Variables

Variables are defined using `::` followed by their type. so creating a global variable named `hi` that is 8 bits looks like,

```
$hi :: i8
```

### access memory identifiers

Getting the location of the mouse might look like,

```
$x :: i16
$y :: i16

fn main() do
  $x = get(.Mouse/x)
  $y = get(.Mouse/y)
end
```

There are a number of memory identifiers that you can access this way,

| Name               | Size |
|--------------------|------|
| .System/vector     | i16  |
| .System/r          | i16  |
| .System/g          | i16  |
| .System/b          | i16  |
| .Console/vector    | i16  |
| .Console/write     | i8   |
| .Screen/vector     | i16  |
| .Screen/width      | i16  |
| .Screen/height     | i16  |
| .Screen/x          | i16  |
| .Screen/y          | i16  |
| .Screen/addr       | i16  |
| .Screen/pixel      | i8   |
| .Screen/sprite     | i8   |
| .Audio{0,3}/addr   | i16  |
| .Audio{0,3}/length | i16  |
| .Audio{0,3}/adsr   | i16  |
| .Audio{0,3}/volume | i16  |
| .Audio{0,3}/pitch  | i16  |
| .Mouse/vector      | i16  |
| .Mouse/x           | i16  |
| .Mouse/y           | i16  |
| .Mouse/state       | i8   |
| .DateTime/year     | i16  |
| .DateTime/month    | i8   |
| .DateTime/hour     | i8   |
| .DateTime/minute   | i8   |
| .DateTime/second   | i8   |

If any are missing, we can add them.

### Do something when something happens

```
fn main() do
  send(.Mouse/vector, :mouse)
end

fn mouse() do
  put("mouse moved!")
end
```


The send method lets you do something whenever the mouse moves.

## Future work

The compiler outputs suboptimal code in a number of ways.
