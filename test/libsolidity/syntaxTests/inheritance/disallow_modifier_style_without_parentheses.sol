contract A { constructor() public { } }
contract B is A { constructor() A public {  } }
// ----
// DeclarationError 1563: (72-73): Modifier-style base constructor call without arguments.
