# Culator

A simple infix notation floating-point cli calculator supporting:

* Simple operations: `+`, `-`, `*`, `/`, `**`/`^`
* Grouping with parentheses: `(`, `)`
* Math constants, e.g.: `pi`, `e`
* Math functions, e.g.: `sqrt`, `sin`, `atan2`, `trunc`

## Examples

```
$ culator
(3 + 2) / 3
1.66666666666667
3 + 2 / 3
3.66666666666667

2**32
4294967296

e
2.71828182845905
pi
3.14159265358979

sin(2*pi)
-1.73436202602476e-34
atan2(1, 1) * 180 / pi
45

e^(1/3 + 2)
10.3122585013258
```

```
$ # Calculate the average line width of FILE
$ culator "`wc -c README.md | cut -d " " -f1` / `wc -l README.md | cut -d " " -f1`"
29.1190476190476
```
# Licensing
MIT or public domain, see [LICENSE](LICENSE).

## References

* Per Vognsen's Bitwise tutorial series [[day 3](https://youtu.be/0woxSWjWsb8)]

