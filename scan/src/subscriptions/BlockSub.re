open ValidatorSub.Mini;

type aggregate_t = {count: int};

type transactions_aggregate_t = {aggregate: option(aggregate_t)};

type internal_t = {
  height: ID.Block.t,
  hash: Hash.t,
  validator: ValidatorSub.Mini.t,
  timestamp: MomentRe.Moment.t,
  transactions_aggregate: transactions_aggregate_t,
};

type t = {
  height: ID.Block.t,
  hash: Hash.t,
  validator: ValidatorSub.Mini.t,
  timestamp: MomentRe.Moment.t,
  txn: int,
};

let toExternal = ({height, hash, validator, timestamp, transactions_aggregate}) => {
  height,
  hash,
  validator,
  timestamp,
  txn:
    switch (transactions_aggregate.aggregate) {
    | Some(aggregate) => aggregate.count
    | _ => 0
    },
};

module MultiConfig = [%graphql
  {|
  subscription Blocks($limit: Int!, $offset: Int!) {
    blocks(limit: $limit, offset: $offset, order_by: {height: desc}) @bsRecord {
      height @bsDecoder(fn: "ID.Block.fromJson")
      hash: block_hash @bsDecoder(fn: "GraphQLParser.hash")
      validator @bsRecord {
        consensusAddress: consensus_address
        operatorAddress: operator_address @bsDecoder(fn: "Address.fromBech32")
        moniker
      }
      timestamp @bsDecoder(fn: "GraphQLParser.time")
      transactions_aggregate @bsRecord {
        aggregate @bsRecord {
          count @bsDecoder(fn: "Belt_Option.getExn")
        }
      }
    }
  }
|}
];

module SingleConfig = [%graphql
  {|
  subscription Block($height: bigint!) {
    blocks_by_pk(height: $height) @bsRecord {
      height @bsDecoder(fn: "ID.Block.fromJson")
      hash: block_hash @bsDecoder(fn: "GraphQLParser.hash")
      validator @bsRecord {
        consensusAddress: consensus_address
        operatorAddress: operator_address @bsDecoder(fn: "Address.fromBech32")
        moniker
      }
      timestamp @bsDecoder(fn: "GraphQLParser.time")
      transactions_aggregate @bsRecord {
        aggregate @bsRecord {
          count @bsDecoder(fn: "Belt_Option.getExn")
        }
      }
    }
  },
|}
];

module BlockCountConfig = [%graphql
  {|
  subscription DataSourcesCount {
    blocks_aggregate{
      aggregate{
        count @bsDecoder(fn: "Belt_Option.getExn")
      }
    }
  }
|}
];

let get = height => {
  let (result, _) =
    ApolloHooks.useSubscription(
      SingleConfig.definition,
      ~variables=SingleConfig.makeVariables(~height=height |> ID.Block.toJson, ()),
    );
  let%Sub x = result;
  switch (x##blocks_by_pk) {
  | Some(data) => Sub.resolve(data |> toExternal)
  | None => NoData
  };
};

let getList = (~page, ~pageSize, ()) => {
  let offset = (page - 1) * pageSize;
  let (result, _) =
    ApolloHooks.useSubscription(
      MultiConfig.definition,
      ~variables=MultiConfig.makeVariables(~limit=pageSize, ~offset, ()),
    );
  result |> Sub.map(_, internal => internal##blocks->Belt_Array.map(toExternal));
};

let count = () => {
  let (result, _) = ApolloHooks.useSubscription(BlockCountConfig.definition);
  result
  |> Sub.map(_, x => x##blocks_aggregate##aggregate |> Belt_Option.getExn |> (y => y##count));
};
