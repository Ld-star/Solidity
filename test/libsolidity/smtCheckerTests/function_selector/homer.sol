pragma experimental SMTChecker;

interface ERC165 {
    /// @notice Query if a contract implements an interface
    /// @param interfaceID The interface identifier, as specified in ERC-165
    /// @dev Interface identification is specified in ERC-165. This function
    ///  uses less than 30,000 gas.
    /// @return `true` if the contract implements `interfaceID` and
    ///  `interfaceID` is not 0xffffffff, `false` otherwise
    function supportsInterface(bytes4 interfaceID) external view returns (bool);
}

interface Simpson {
    function is2D() external returns (bool);
    function skinColor() external returns (string memory);
}

interface PeaceMaker {
    function achieveWorldPeace() external;
}

contract Homer is ERC165, Simpson {
    function supportsInterface(bytes4 interfaceID) public pure override returns (bool) {
        return
            interfaceID == this.supportsInterface.selector || // ERC165
            interfaceID == this.is2D.selector ^ this.skinColor.selector; // Simpson
    }

    function is2D() external pure override returns (bool) {
        return true;
    }

    function skinColor() external pure override returns (string memory) {
        return "yellow";
    }

    function check() public pure {
        assert(supportsInterface(type(Simpson).interfaceId));
        assert(supportsInterface(type(ERC165).interfaceId));
        assert(supportsInterface(type(PeaceMaker).interfaceId));
    }
}


// ----
// Warning 6328: (1373-1428): CHC: Assertion violation happens here.\nCounterexample:\n\n\nTransaction trace:\nHomer.constructor()\nHomer.check()\n  Homer.supportsInterface(1941353618) -- internal call\n  Homer.supportsInterface(33540519) -- internal call\n  Homer.supportsInterface(2342435274) -- internal call
