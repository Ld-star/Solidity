contract D { constructor() payable {} }
contract C {
	uint public x;
	constructor() payable {}
	function f(uint amount) public returns (D) {
		x++;
		return (new D){value: amount}();
	}
	function stack(uint depth) public payable returns (address) {
		if (depth > 0)
			return this.stack(depth - 1);
		else
			return address(f(0));
	}
}
// ====
// EVMVersion: >=byzantium
// compileViaYul: also
// ----
// constructor(), 20 wei
// gas irOptimized: 255579
// gas legacy: 285485
// gas legacyOptimized: 177933
// f(uint256): 20 -> 1370859564726510389319704988634906228201275401179
// x() -> 1
// f(uint256): 20 -> FAILURE
// x() -> 1
// stack(uint256): 1023 -> FAILURE
// gas irOptimized: 856335
// gas legacy: 981671
// gas legacyOptimized: 824895
// x() -> 1
// stack(uint256): 10 -> 693016686122178122849713379390321835634789309880
// x() -> 2
