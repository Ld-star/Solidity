contract C {
    function f() public {
        keccak256.gas();
    }
}
// ----
// TypeError: (47-60): Member "gas" not found or not visible after argument-dependent lookup in function (bytes memory) pure returns (bytes32)
