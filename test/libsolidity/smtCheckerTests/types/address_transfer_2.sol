contract C
{
	constructor() payable {}
	function f(uint x, address payable a, address payable b) public {
		require(a != b);
		require(x == 100);
		require(x == a.balance);
		require(a.balance == b.balance);
		a.transfer(600);
		b.transfer(100);
		// Fails since a == this is possible.
		assert(a.balance > b.balance);
	}
}
// ====
// SMTEngine: all
// SMTIgnoreCex: yes
// ----
// Warning 6328: (288-317): CHC: Assertion violation happens here.
// Warning 1236: (210-225): BMC: Insufficient funds happens here.
// Warning 1236: (229-244): BMC: Insufficient funds happens here.
