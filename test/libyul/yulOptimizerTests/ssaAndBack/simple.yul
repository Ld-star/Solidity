{
    let a := mload(0)
    a := mload(1)
    mstore(a, 0)
}
// ----
// ssaAndBack
// {
//     pop(mload(0))
//     let a_2 := mload(1)
//     mstore(a_2, 0)
// }
