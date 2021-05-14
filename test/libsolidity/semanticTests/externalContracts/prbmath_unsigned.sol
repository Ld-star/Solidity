==== ExternalSource: _prbmath/PRBMathCommon.sol ====
==== ExternalSource: _prbmath/PRBMathUD60x18.sol ====
==== Source: prbmath.sol ====
import "_prbmath/PRBMathUD60x18.sol";

contract test {
    using PRBMathUD60x18 for uint256;

    function div(uint256 x, uint256 y) external pure returns (uint256 ret) {
        ret = x.div(y);
    }
    function exp(uint256 x) external pure returns (uint256 ret) {
        ret = x.exp();
    }
    function exp2(uint256 x) external pure returns (uint256 ret) {
        ret = x.exp2();
    }
    function gm(uint256 x, uint256 y) external pure returns (uint256 ret) {
        ret = x.gm(y);
    }
    function log10(uint256 x) external pure returns (uint256 ret) {
        ret = x.log10();
    }
    function log2(uint256 x) external pure returns (uint256 ret) {
        ret = x.log2();
    }
    function mul(uint256 x, uint256 y) external pure returns (uint256 ret) {
        ret = x.mul(y);
    }
    function pow(uint256 x, uint256 y) external pure returns (uint256 ret) {
        ret = x.pow(y);
    }
    function sqrt(uint256 x) external pure returns (uint256 ret) {
        ret = x.sqrt();
    }
    function benchmark(uint256 x) external pure returns (uint256 ret, uint256 z1, uint256 z2) {
        uint256 y = x.mul(3).ceil();
        uint256 z = y.div(x);
        for (uint i = 0; i < 10; i++)
            z = z.sqrt();
        ret = z;

        // Check precision
        z1 = z.ceil();
        z2 = z.sqrt().pow(2).ceil();
        assert(z1 == z2);
    }
}
// ====
// compileViaYul: also
// ----
// constructor()
// gas irOptimized: 1961108
// gas legacy: 2356230
// gas legacyOptimized: 1770105
// div(uint256,uint256): 3141592653589793238, 88714123 -> 35412542528203691288251815328
// gas irOptimized: 22103
// gas legacy: 22497
// gas legacyOptimized: 22010
// exp(uint256): 3141592653589793238 -> 23140692632779268978
// gas irOptimized: 24981
// gas legacy: 25104
// gas legacyOptimized: 24258
// exp2(uint256): 3141592653589793238 -> 8824977827076287620
// gas irOptimized: 24746
// gas legacy: 24814
// gas legacyOptimized: 24062
// gm(uint256,uint256): 3141592653589793238, 88714123 -> 16694419339601
// gas irOptimized: 23575
// gas legacy: 23269
// gas legacyOptimized: 22724
// log10(uint256): 3141592653589793238 -> 0x44fe4fc084a52b8a
// gas irOptimized: 30936
// gas legacy: 32898
// gas legacyOptimized: 29925
// log2(uint256): 3141592653589793238 -> 1651496129472318782
// gas irOptimized: 28838
// gas legacy: 30986
// gas legacyOptimized: 28001
// mul(uint256,uint256): 3141592653589793238, 88714123 -> 278703637
// gas irOptimized: 22146
// gas legacy: 22604
// gas legacyOptimized: 22090
// pow(uint256,uint256): 3141592653589793238, 5 -> 306019684785281453040
// gas irOptimized: 22653
// gas legacy: 23245
// gas legacyOptimized: 22646
// sqrt(uint256): 3141592653589793238 -> 1772453850905516027
// gas irOptimized: 23264
// gas legacy: 22820
// gas legacyOptimized: 22440
// benchmark(uint256): 3141592653589793238 -> 998882724338592125, 1000000000000000000, 1000000000000000000
// gas irOptimized: 42063
// gas legacy: 35385
// gas legacyOptimized: 33449
