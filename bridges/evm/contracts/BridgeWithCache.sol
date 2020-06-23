pragma solidity 0.5.14;
pragma experimental ABIEncoderV2;

import {Packets} from "./Packets.sol";
import {Bridge} from "./Bridge.sol";

/// @title BridgeWithCache <3 BandChain
/// @author Band Protocol Team
contract BridgeWithCache is Bridge {
    /// Mapping from hash of RequestPacket to the latest ResponsePacket.
    mapping(bytes32 => ResponsePacket) public requestsCache;

    /// Initializes an oracle bridge to BandChain by pass the argument to the parent contract (Bridge.sol).
    /// @param _validators The initial set of BandChain active validators.
    constructor(ValidatorWithPower[] memory _validators)
        public
        Bridge(_validators)
    {}

    /// Create hash of a RequestPacket.
    /// @param _request A tuple that represent RequestPacket struct.
    function getRequestKey(RequestPacket memory _request)
        public
        pure
        returns (bytes32)
    {
        return keccak256(abi.encode(_request));
    }

    /// Query the ResponsePacket for a given RequestPacket.
    /// Revert if can't find the related response in the mapping.
    /// @param _request A tuple that represent RequestPacket struct.
    function getLatestResponse(RequestPacket memory _request)
        public
        view
        returns (ResponsePacket memory)
    {
        ResponsePacket memory res = requestsCache[keccak256(
            abi.encode(_request)
        )];
        require(res.requestId != 0, "RESPONSE_NOT_FOUND");

        return res;
    }

    /// Performs oracle state relay and oracle data verification in one go.
    /// After that, the results will be recorded to the state by using the hash of RequestPacket as key.
    /// @param _data The encoded data for oracle state relay and data verification.
    function relay(bytes calldata _data) external {
        (RequestPacket memory req, ResponsePacket memory res) = relayAndVerify(
            _data
        );

        requestsCache[keccak256(abi.encode(req))] = res;
    }
}
