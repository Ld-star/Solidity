{
  mstore(10, 11)
}
// ----
// Trace:
//   MSTORE_AT_SIZE(10, 32) [000000000000000000000000000000000000000000000000000000000000000b]
// Memory dump:
//     20: 0000000000000000000b00000000000000000000000000000000000000000000
// Storage dump:
