pragma experimental SMTChecker;

contract C
{
	uint x;
	mapping (uint => uint) map;
	function f(address a, bytes memory data) public {
		x = 0;
		map[0] = 0;
		mapping (uint => uint) storage localMap = map;
		(bool success, bytes memory ret) = a.call(data);
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
// Warning 4588: (244-256): Assertion checker does not yet implement this type of function call.
// Warning 6328: (260-275): CHC: Assertion violation happens here.\nCounterexample:\nx = 0\na = 0\n\nTransaction trace:\nC.constructor()\nState: x = 0\nC.f(0, data)
// Warning 6328: (279-293): CHC: Assertion violation happens here.\nCounterexample:\nx = 1\na = 0\n\nTransaction trace:\nC.constructor()\nState: x = 0\nC.f(0, data)
// Warning 6328: (297-316): CHC: Assertion violation happens here.\nCounterexample:\nx = 1\na = 0\n\nTransaction trace:\nC.constructor()\nState: x = 0\nC.f(0, data)
// Warning 6328: (320-344): CHC: Assertion violation happens here.\nCounterexample:\nx = 1\na = 0\n\nTransaction trace:\nC.constructor()\nState: x = 0\nC.f(0, data)
// Warning 4588: (244-256): Assertion checker does not yet implement this type of function call.
