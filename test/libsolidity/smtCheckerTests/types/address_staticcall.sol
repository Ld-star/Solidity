pragma experimental SMTChecker;

contract C
{
	uint x;
	mapping (uint => uint) map;
	function f(address a, bytes memory data) public {
		x = 0;
		map[0] = 0;
		mapping (uint => uint) storage localMap = map;
		(bool success, bytes memory ret) = a.staticcall(data);
		assert(success);
		assert(x == 0);
		assert(map[0] == 0);
		assert(localMap[0] == 0);
	}
}
// ====
// EVMVersion: >spuriousDragon
// ----
// Warning 2072: (224-240): Unused local variable.
// Warning 6328: (266-281): CHC: Assertion violation happens here.\nCounterexample:\nx = 0\na = 0\ndata = [7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 14, 7, 7, 7, 19, 7, 7, 7, 7, 7, 7, 7, 27, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7]\n\n\nTransaction trace:\nC.constructor()\nState: x = 0\nC.f(0, [7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 14, 7, 7, 7, 19, 7, 7, 7, 7, 7, 7, 7, 27, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7])
