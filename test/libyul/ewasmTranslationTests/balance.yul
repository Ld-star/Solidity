{
  sstore(0, balance(address()))
  sstore(1, balance(0))
}
// ----
// Trace:
// Memory dump:
//      0: 0000000000000000000000000000000000000000000000000000000000000001
//     20: 0000000000000000000000000000000022222222000000000000000000000000
// Storage dump:
//   0000000000000000000000000000000000000000000000000000000000000000: 0000000000000000000000000000000022222222000000000000000000000000
//   0000000000000000000000000000000000000000000000000000000000000001: 0000000000000000000000000000000022222222000000000000000000000000
