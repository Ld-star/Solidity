==== Source: a ====
contract A {}
==== Source: b ====
import {A} from "a"; contract A {}
// ----
// DeclarationError 2333: (b:21-34): Identifier already declared.
