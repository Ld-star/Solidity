{
    let b := 20
    stop()
    for {
        let a := 20
    }
    lt(a, 40)
    {
        a := add(a, 2)
    }
    {
        a := a
        mstore(0, a)
        a := add(a, 10)
    }
}

// ====
// step: deadCodeEliminator
// ----
// {
//     let b := 20
//     stop()
// }
