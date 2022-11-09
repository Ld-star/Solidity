contract C {
    uint[] storageArray;
    function set_get_length(uint256 len) public returns (uint256) {
        while(storageArray.length < len)
            storageArray.push();
        while(storageArray.length > 0)
            storageArray.pop();
        return storageArray.length;
    }
}
// ----
// set_get_length(uint256): 0 -> 0
// set_get_length(uint256): 1 -> 0
// set_get_length(uint256): 10 -> 0
// set_get_length(uint256): 20 -> 0
// gas irOptimized: 106222
// gas legacy: 105722
// gas legacyOptimized: 103508
// set_get_length(uint256): 0xFF -> 0
// gas irOptimized: 821872
// gas legacy: 810327
// gas legacyOptimized: 786258
// set_get_length(uint256): 0xFFF -> 0
// gas irOptimized: 12841084
// gas legacy: 12649059
// gas legacyOptimized: 12267870
// set_get_length(uint256): 0xFFFF -> FAILURE # Out-of-gas #
