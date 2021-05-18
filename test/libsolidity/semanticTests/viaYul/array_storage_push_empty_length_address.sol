contract C {
    address[] addressArray;
    function set_get_length(uint256 len) public returns (uint256)
    {
        while(addressArray.length < len)
            addressArray.push();
        while(addressArray.length > len)
            addressArray.pop();
        return addressArray.length;
    }
}
// ====
// EVMVersion: >=petersburg
// compileViaYul: also
// ----
// set_get_length(uint256): 0 -> 0
// set_get_length(uint256): 1 -> 1
// set_get_length(uint256): 10 -> 10
// set_get_length(uint256): 20 -> 20
// set_get_length(uint256): 0 -> 0
// gas irOptimized: 79199
// gas legacy: 35730
// gas legacyOptimized: 77162
// set_get_length(uint256): 0xFF -> 0xFF
// gas irOptimized: 155961
// gas legacy: 636237
// gas legacyOptimized: 115104
// set_get_length(uint256): 0xFFF -> 0xFFF
// gas irOptimized: 2013003
// gas legacy: 9871774
// gas legacyOptimized: 1398546
// set_get_length(uint256): 0xFFFF -> 0xffff # Out-of-gas #
// gas irOptimized: 31849803
// gas legacyOptimized: 22019346
