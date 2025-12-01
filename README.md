# fcomp - a file comparison program

The algorithm and application were published [here](./a-file-comparison-program.pdf).

Source code (almost identical) with the original published version is in file [fcomp.c](./fcomp.c).

To compile the original version, use the following command:

```shell
$ gcc -std=c89 fcomp.c -o fcomp
```

Then you can try how it works:

```shell
$ ./fcomp file1.txt file2.txt
```

Expected output:

```text
inserted after line 0:
 b
 x
 x
```
