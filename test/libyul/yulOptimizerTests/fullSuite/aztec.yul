/**
 * @title Library to validate AZTEC zero-knowledge proofs
 * @author Zachary Williamson, AZTEC
 * @dev Don't include this as an internal library. This contract uses a static memory table to cache elliptic curve primitives and hashes.
 * Calling this internally from another function will lead to memory mutation and undefined behaviour.
 * The intended use case is to call this externally via `staticcall`. External calls to OptimizedAZTEC can be treated as pure functions as this contract contains no storage and makes no external calls (other than to precompiles)
 * Copyright Spilbury Holdings Ltd 2018. All rights reserved.
 * We will be releasing AZTEC as an open-source protocol that provides efficient transaction privacy for Ethereum.
 * This will include our bespoke AZTEC decentralized exchange, allowing for cross-asset transfers with full transaction privacy
 * and interopability with public decentralized exchanges.
 * Stay tuned for updates!
 *
 * Permission to use as test case in the Solidity compiler granted by the author:
 * https://github.com/ethereum/solidity/pull/5713#issuecomment-449042830
**/
{
    validateJoinSplit()
    // should not get here
    mstore(0x00, 404)
    revert(0x00, 0x20)


    function validateJoinSplit() {
        mstore(0x80, 7673901602397024137095011250362199966051872585513276903826533215767972925880) // h_x
        mstore(0xa0, 8489654445897228341090914135473290831551238522473825886865492707826370766375) // h_y
        let notes := add(0x04, calldataload(0x04))
        let m := calldataload(0x24)
        let n := calldataload(notes)
        let gen_order := 0x30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000001
        let challenge := mod(calldataload(0x44), gen_order)

        // validate m <= n
        if gt(m, n) { mstore(0x00, 404) revert(0x00, 0x20) }

        // recover k_{public} and calculate k_{public}
        let kn := calldataload(sub(calldatasize(), 0xc0))

        // add kn and m to final hash table
        mstore(0x2a0, caller())
        mstore(0x2c0, kn)
        mstore(0x2e0, m)
        kn := mulmod(sub(gen_order, kn), challenge, gen_order) // we actually want c*k_{public}
        hashCommitments(notes, n)
        let b := add(0x300, mul(n, 0x80))

        // Iterate over every note and calculate the blinding factor B_i = \gamma_i^{kBar}h^{aBar}\sigma_i^{-c}.
        // We use the AZTEC protocol pairing optimization to reduce the number of pairing comparisons to 1, which adds some minor alterations
        for { let i := 0 } lt(i, n) { i := add(i, 0x01) } {

            // Get the calldata index of this note
            let noteIndex := add(add(notes, 0x20), mul(i, 0xc0))


            let k
            let a := calldataload(add(noteIndex, 0x20))
            let c := challenge

            switch eq(add(i, 0x01), n)
            case 1 {
                k := kn

                // if all notes are input notes, invert k
                if eq(m, n) {
                    k := sub(gen_order, k)
                }
            }
            case 0 { k := calldataload(noteIndex) }

            // Check this commitment is well formed...
            validateCommitment(noteIndex, k, a)

            // If i > m then this is an output note.
            // Set k = kx_j, a = ax_j, c = cx_j, where j = i - (m+1)
            switch gt(add(i, 0x01), m)
            case 1 {

                // before we update k, update kn = \sum_{i=0}^{m-1}k_i - \sum_{i=m}^{n-1}k_i
                kn := addmod(kn, sub(gen_order, k), gen_order)
                let x := mod(mload(0x00), gen_order)
                k := mulmod(k, x, gen_order)
                a := mulmod(a, x, gen_order)
                c := mulmod(challenge, x, gen_order)

                // calculate x_{j+1}
                mstore(0x00, keccak256(0x00, 0x20))
            }
            case 0 {

                // nothing to do here except update kn = \sum_{i=0}^{m-1}k_i - \sum_{i=m}^{n-1}k_i
                kn := addmod(kn, k, gen_order)
            }

            calldatacopy(0xe0, add(noteIndex, 0x80), 0x40)
            calldatacopy(0x20, add(noteIndex, 0x40), 0x40)
            mstore(0x120, sub(gen_order, c))
            mstore(0x60, k)
            mstore(0xc0, a)

            // Using call instead of staticcall here to make it work on all targets.
            let result := call(gas(), 7, 0, 0xe0, 0x60, 0x1a0, 0x40)
            result := and(result, call(gas(), 7, 0, 0x20, 0x60, 0x120, 0x40))
            result := and(result, call(gas(), 7, 0, 0x80, 0x60, 0x160, 0x40))

            result := and(result, call(gas(), 6, 0, 0x120, 0x80, 0x160, 0x40))

            result := and(result, call(gas(), 6, 0, 0x160, 0x80, b, 0x40))

            if eq(i, m) {
                mstore(0x260, mload(0x20))
                mstore(0x280, mload(0x40))
                mstore(0x1e0, mload(0xe0))
                mstore(0x200, sub(0x30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd47, mload(0x100)))
            }

            if gt(i, m) {
                mstore(0x60, c)
                result := and(result, call(gas(), 7, 0, 0x20, 0x60, 0x220, 0x40))

                result := and(result, call(gas(), 6, 0, 0x220, 0x80, 0x260, 0x40))
                result := and(result, call(gas(), 6, 0, 0x1a0, 0x80, 0x1e0, 0x40))
            }

            if iszero(result) { mstore(0x00, 400) revert(0x00, 0x20) }
            b := add(b, 0x40) // increase B pointer by 2 words
        }

        if lt(m, n) {
            validatePairing(0x64)
        }

        let expected := mod(keccak256(0x2a0, sub(b, 0x2a0)), gen_order)
        if iszero(eq(expected, challenge)) {

            // No! Bad! No soup for you!
            mstore(0x00, 404)
            revert(0x00, 0x20)
        }

        // Great! All done. This is a valid proof so return ```true```
        mstore(0x00, 0x01)
        return(0x00, 0x20)
    }

    function validatePairing(t2) {
        let field_order := 0x30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd47
        let t2_x_1 := calldataload(t2)
        let t2_x_2 := calldataload(add(t2, 0x20))
        let t2_y_1 := calldataload(add(t2, 0x40))
        let t2_y_2 := calldataload(add(t2, 0x60))

        // check provided setup pubkey is not zero or g2
        if or(or(or(or(or(or(or(
            iszero(t2_x_1),
            iszero(t2_x_2)),
            iszero(t2_y_1)),
            iszero(t2_y_2)),
            eq(t2_x_1, 0x1800deef121f1e76426a00665e5c4479674322d4f75edadd46debd5cd992f6ed)),
            eq(t2_x_2, 0x198e9393920d483a7260bfb731fb5d25f1aa493335a9e71297e485b7aef312c2)),
            eq(t2_y_1, 0x12c85ea5db8c6deb4aab71808dcb408fe3d1e7690c43d37b4ce6cc0166fa7daa)),
            eq(t2_y_2, 0x90689d0585ff075ec9e99ad690c3395bc4b313370b38ef355acdadcd122975b))
        {
            mstore(0x00, 400)
            revert(0x00, 0x20)
        }

        mstore(0x20, mload(0x1e0)) // sigma accumulator x
        mstore(0x40, mload(0x200)) // sigma accumulator y
        mstore(0x80, 0x1800deef121f1e76426a00665e5c4479674322d4f75edadd46debd5cd992f6ed)
        mstore(0x60, 0x198e9393920d483a7260bfb731fb5d25f1aa493335a9e71297e485b7aef312c2)
        mstore(0xc0, 0x12c85ea5db8c6deb4aab71808dcb408fe3d1e7690c43d37b4ce6cc0166fa7daa)
        mstore(0xa0, 0x90689d0585ff075ec9e99ad690c3395bc4b313370b38ef355acdadcd122975b)
        mstore(0xe0, mload(0x260)) // gamma accumulator x
        mstore(0x100, mload(0x280)) // gamma accumulator y
        mstore(0x140, t2_x_1)
        mstore(0x120, t2_x_2)
        mstore(0x180, t2_y_1)
        mstore(0x160, t2_y_2)

        let success := call(gas(), 8, 0, 0x20, 0x180, 0x20, 0x20)

        if or(iszero(success), iszero(mload(0x20))) {
            mstore(0x00, 400)
            revert(0x00, 0x20)
        }
    }

    function validateCommitment(note, k, a) {
        let gen_order := 0x30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000001
        let field_order := 0x30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd47
        let gammaX := calldataload(add(note, 0x40))
        let gammaY := calldataload(add(note, 0x60))
        let sigmaX := calldataload(add(note, 0x80))
        let sigmaY := calldataload(add(note, 0xa0))
        if iszero(
            and(
                and(
                    and(
                        eq(mod(a, gen_order), a), // a is modulo generator order?
                        gt(a, 1)                  // can't be 0 or 1 either!
                    ),
                    and(
                        eq(mod(k, gen_order), k), // k is modulo generator order?
                        gt(k, 1)                  // and not 0 or 1
                    )
                ),
                and(
                    eq( // y^2 ?= x^3 + 3
                        addmod(mulmod(mulmod(sigmaX, sigmaX, field_order), sigmaX, field_order), 3, field_order),
                        mulmod(sigmaY, sigmaY, field_order)
                    ),
                    eq( // y^2 ?= x^3 + 3
                        addmod(mulmod(mulmod(gammaX, gammaX, field_order), gammaX, field_order), 3, field_order),
                        mulmod(gammaY, gammaY, field_order)
                    )
                )
            )
        ) {
            mstore(0x00, 400)
            revert(0x00, 0x20)
        }
    }

    function hashCommitments(notes, n) {
        for { let i := 0 } lt(i, n) { i := add(i, 0x01) } {
            let index := add(add(notes, mul(i, 0xc0)), 0x60)
            calldatacopy(add(0x300, mul(i, 0x80)), index, 0x80)
        }
        mstore(0x00, keccak256(0x300, mul(n, 0x80)))
    }
}
// ----
// fullSuite
// {
//     let validateJo := 0x80
//     mstore(validateJo, 7673901602397024137095011250362199966051872585513276903826533215767972925880)
//     mstore(0xa0, 8489654445897228341090914135473290831551238522473825886865492707826370766375)
//     let validateJo_m := calldataload(0x24)
//     let validateJo_n := calldataload(add(0x04, calldataload(0x04)))
//     let validateJo_gen_order := 0x30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000001
//     let validateJo_challenge := mod(calldataload(0x44), validateJo_gen_order)
//     if gt(validateJo_m, validateJo_n)
//     {
//         mstore(0x00, 404)
//         revert(0x00, 0x20)
//     }
//     let validateJo_kn := calldataload(add(calldatasize(), 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff40))
//     mstore(0x2a0, caller())
//     mstore(0x2c0, validateJo_kn)
//     mstore(0x2e0, validateJo_m)
//     validateJo_kn := mulmod(sub(validateJo_gen_order, validateJo_kn), validateJo_challenge, validateJo_gen_order)
//     hashCommitments(add(0x04, calldataload(0x04)), validateJo_n)
//     let validateJo_b := add(0x300, mul(validateJo_n, validateJo))
//     let validateJo_i := 0
//     let validateJo_i_1 := validateJo_i
//     for {
//     }
//     lt(validateJo_i, validateJo_n)
//     {
//         validateJo_i := add(validateJo_i, 0x01)
//     }
//     {
//         let validateJo_1 := add(calldataload(0x04), mul(validateJo_i, 0xc0))
//         let validateJo_k := validateJo_i_1
//         let validateJo_a := calldataload(add(validateJo_1, 0x44))
//         let validateJo_c := validateJo_challenge
//         switch eq(add(validateJo_i, 0x01), validateJo_n)
//         case 1 {
//             validateJo_k := validateJo_kn
//             if eq(validateJo_m, validateJo_n)
//             {
//                 validateJo_k := sub(validateJo_gen_order, validateJo_kn)
//             }
//         }
//         case 0 {
//             validateJo_k := calldataload(add(validateJo_1, 0x24))
//         }
//         validateCommitment(add(validateJo_1, 0x24), validateJo_k, validateJo_a)
//         switch gt(add(validateJo_i, 0x01), validateJo_m)
//         case 1 {
//             validateJo_kn := addmod(validateJo_kn, sub(validateJo_gen_order, validateJo_k), validateJo_gen_order)
//             let validateJo_x := mod(mload(validateJo_i_1), validateJo_gen_order)
//             validateJo_k := mulmod(validateJo_k, validateJo_x, validateJo_gen_order)
//             validateJo_a := mulmod(validateJo_a, validateJo_x, validateJo_gen_order)
//             validateJo_c := mulmod(validateJo_challenge, validateJo_x, validateJo_gen_order)
//             mstore(validateJo_i_1, keccak256(validateJo_i_1, 0x20))
//         }
//         case 0 {
//             validateJo_kn := addmod(validateJo_kn, validateJo_k, validateJo_gen_order)
//         }
//         let validateJo_2 := 0x40
//         calldatacopy(0xe0, add(validateJo_1, 164), validateJo_2)
//         calldatacopy(0x20, add(validateJo_1, 100), validateJo_2)
//         mstore(0x120, sub(validateJo_gen_order, validateJo_c))
//         mstore(0x60, validateJo_k)
//         mstore(0xc0, validateJo_a)
//         let validateJo_result := call(gas(), 7, validateJo_i_1, 0xe0, 0x60, 0x1a0, validateJo_2)
//         let validateJo_result_1 := and(validateJo_result, call(gas(), 7, validateJo_i_1, 0x20, 0x60, 0x120, validateJo_2))
//         let validateJo_result_2 := and(validateJo_result_1, call(gas(), 7, validateJo_i_1, validateJo, 0x60, 0x160, validateJo_2))
//         let validateJo_result_3 := and(validateJo_result_2, call(gas(), 6, validateJo_i_1, 0x120, validateJo, 0x160, validateJo_2))
//         validateJo_result := and(validateJo_result_3, call(gas(), 6, validateJo_i_1, 0x160, validateJo, validateJo_b, validateJo_2))
//         if eq(validateJo_i, validateJo_m)
//         {
//             mstore(0x260, mload(0x20))
//             mstore(0x280, mload(validateJo_2))
//             mstore(0x1e0, mload(0xe0))
//             mstore(0x200, sub(0x30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd47, mload(0x100)))
//         }
//         if gt(validateJo_i, validateJo_m)
//         {
//             mstore(0x60, validateJo_c)
//             let validateJo_result_4 := and(validateJo_result, call(gas(), 7, validateJo_i_1, 0x20, 0x60, 0x220, validateJo_2))
//             let validateJo_result_5 := and(validateJo_result_4, call(gas(), 6, validateJo_i_1, 0x220, validateJo, 0x260, validateJo_2))
//             validateJo_result := and(validateJo_result_5, call(gas(), 6, validateJo_i_1, 0x1a0, validateJo, 0x1e0, validateJo_2))
//         }
//         if iszero(validateJo_result)
//         {
//             mstore(validateJo_i_1, 400)
//             revert(validateJo_i_1, 0x20)
//         }
//         validateJo_b := add(validateJo_b, validateJo_2)
//     }
//     if lt(validateJo_m, validateJo_n)
//     {
//         validatePairing(0x64)
//     }
//     if iszero(eq(mod(keccak256(0x2a0, add(validateJo_b, 0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffd60)), validateJo_gen_order), validateJo_challenge))
//     {
//         mstore(validateJo_i_1, 404)
//         revert(validateJo_i_1, 0x20)
//     }
//     mstore(validateJo_i_1, 0x01)
//     return(validateJo_i_1, 0x20)
//     mstore(validateJo_i_1, 404)
//     revert(validateJo_i_1, 0x20)
//     function validatePairing(t2)
//     {
//         let t2_x := calldataload(t2)
//         let _1 := 0x20
//         let t2_x_1 := calldataload(add(t2, _1))
//         let t2_y := calldataload(add(t2, 0x40))
//         let t2_y_1 := calldataload(add(t2, 0x60))
//         let _2 := 0x90689d0585ff075ec9e99ad690c3395bc4b313370b38ef355acdadcd122975b
//         let _3 := 0x12c85ea5db8c6deb4aab71808dcb408fe3d1e7690c43d37b4ce6cc0166fa7daa
//         let _4 := 0x198e9393920d483a7260bfb731fb5d25f1aa493335a9e71297e485b7aef312c2
//         let _5 := 0x1800deef121f1e76426a00665e5c4479674322d4f75edadd46debd5cd992f6ed
//         if or(or(or(or(or(or(or(iszero(t2_x), iszero(t2_x_1)), iszero(t2_y)), iszero(t2_y_1)), eq(t2_x, _5)), eq(t2_x_1, _4)), eq(t2_y, _3)), eq(t2_y_1, _2))
//         {
//             mstore(0x00, 400)
//             revert(0x00, _1)
//         }
//         mstore(_1, mload(0x1e0))
//         mstore(0x40, mload(0x200))
//         mstore(0x80, _5)
//         mstore(0x60, _4)
//         mstore(0xc0, _3)
//         mstore(0xa0, _2)
//         mstore(0xe0, mload(0x260))
//         mstore(0x100, mload(0x280))
//         mstore(0x140, t2_x)
//         mstore(0x120, t2_x_1)
//         let _6 := 0x180
//         mstore(_6, t2_y)
//         mstore(0x160, t2_y_1)
//         let success := call(gas(), 8, 0, _1, _6, _1, _1)
//         if or(iszero(success), iszero(mload(_1)))
//         {
//             mstore(0, 400)
//             revert(0, _1)
//         }
//     }
//     function validateCommitment(note, k, a)
//     {
//         let gen_order := 0x30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000001
//         let field_order := 0x30644e72e131a029b85045b68181585d97816a916871ca8d3c208c16d87cfd47
//         let gammaX := calldataload(add(note, 0x40))
//         let gammaY := calldataload(add(note, 0x60))
//         let sigmaX := calldataload(add(note, 0x80))
//         let sigmaY := calldataload(add(note, 0xa0))
//         if iszero(and(and(and(eq(mod(a, gen_order), a), gt(a, 1)), and(eq(mod(k, gen_order), k), gt(k, 1))), and(eq(addmod(mulmod(mulmod(sigmaX, sigmaX, field_order), sigmaX, field_order), 3, field_order), mulmod(sigmaY, sigmaY, field_order)), eq(addmod(mulmod(mulmod(gammaX, gammaX, field_order), gammaX, field_order), 3, field_order), mulmod(gammaY, gammaY, field_order)))))
//         {
//             mstore(0x00, 400)
//             revert(0x00, 0x20)
//         }
//     }
//     function hashCommitments(notes, n)
//     {
//         let i := 0
//         for {
//         }
//         lt(i, n)
//         {
//             i := add(i, 0x01)
//         }
//         {
//             calldatacopy(add(0x300, mul(i, 0x80)), add(add(notes, mul(i, 0xc0)), 0x60), 0x80)
//         }
//         mstore(0, keccak256(0x300, mul(n, 0x80)))
//     }
// }
