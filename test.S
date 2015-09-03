
linkaddr_2:
  jal target_2
  nop
  nop

target_2:
  la  x2, foo_2
  jalr x2

foo2:
  nop
  nop
  j 1f
1:
  nop
  nop
  j 2f
  nop
  nop
  nop
  nop
2:
  j 1f
  j 1f
  j 1f
  j 1f
1:
  nop
  nop
  nop
  nop
  nop
  nop
  j 1f
  nop
  nop
  j 1f
  nop
  nop
  nop
  nop
  nop
  nop
1:
  j linkaddr_2
  nop
  nop
  nop
  nop
