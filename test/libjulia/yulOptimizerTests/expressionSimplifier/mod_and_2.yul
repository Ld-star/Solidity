{
    mstore(0, mod(calldataload(0), exp(2, 255)))
}
// ----
// expressionSimplifier
// {
//     mstore(0, and(calldataload(0), 0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff))
// }
