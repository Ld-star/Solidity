{
  sstore(0, call(gas(), address(), 42, 0, 0x20, 0x20, 0x20))
}
// ----
// Trace:
//   CALL()
// Memory dump:
//     20: 0000000000000000000000000000000000000000000000000000000000000001
// Storage dump:
//   0000000000000000000000000000000000000000000000000000000000000000: 0000000000000000000000000000000000000000000000000000000000000001
