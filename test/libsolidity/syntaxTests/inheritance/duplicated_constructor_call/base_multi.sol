contract C { constructor(uint) public {} }
contract A is C(2) {}
contract B is C(2) {}
contract D is A, B { constructor() C(3) public {} }
// ----
// Warning: Base constructor arguments given twice.
// Warning: Base constructor arguments given twice.
