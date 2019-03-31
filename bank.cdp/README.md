
###### Future note: some of the information herein should be apt for re-organization into the relevant Ricardian clauses of the bank.cdp contract ABI

# Design Decisions 

* If you'd like your CDP to dip below the agreed upon liqidation ratio of your CDP's type, via one of the relevant actions (draw or bail), then TOO BAD because you won't be permitted (you'll hit an assertion).
* If you'd like to liquidate your own CDP in the event of significant price slippage, then TOO BAD because that would let you print free money and we won't have that kind of behavior here (unless you collude with another account).

# Differences between cdpEOS (aka bank.cdp) and cdpETH (aka MakerDAO)

bank.cdp successfully implements multi-collateral CDPs by accepting any eosio.token, and the entire protocol is implemented via only 15 distinct functions, and just over 800 SLOC. 

The major discrepancy between the two protocols is in how
liquidation auctions, stability fees, and governance are handled. Nonetheless, there are many concepts in common, such as the fact that liquidation may (and should) operate effectively in the absence of price feeds.

To provide for better liquidity, Maker uses simultaneous (parallel) auctions of four types:

* #1 Dai (stable token) bids for collateral
* #2 Reverse dutch of the above (collateral bids for Dai) in the event of some collateral remaining to be refunded to a CDP owner.
* #3 Dai bids for MKR (voting token) in the event of balance shortage in #1 (not enough Dai was raised)
* #4 MKR bids for Dai in the event of balance surplus in #1 (too much Dai was raised) 

Auction #2 is omitted entirely from our protocol: we do not endorse it because CDP owners should be playing it safe and overcollateralizing even more. The rest of the phases are executed in sequence rather than in parallel. 

Unlike in Ethereum where the primary constraint is speed and gas, with faster block times the main constraint in EOS is RAM so our approach was taken from the angle of a more space efficient implementation, and a simpler view of the computation overall. 

On that last note, we also omit the calculation of stability fees on a global basis as in multi-collateral Dai (whenever debt is being taken on or repaid, the fee is minted into the contract's balance and incremented onto a CDP's debt), in favor of an APR-based deduction in VTO upon every wipe action executed on a CDP (closer to single-collateral Dai). Moreover, our voting and referendum procedure has a very common sense nature:

Any account may create a proposal, and as many varying proposals as desired, but we do constrain proposals per CDP type. Only one proposal may be active (in voting) per CDP type (new or modification of existing).

Voters may vote with their tokens as much as they please (a la Maker) and even post two postions simultaneously (both for and against).

All voting tokens are refunded to voters upon the expiration of the voting period for a proposal, and if there is a tie between total for and against positions, another voting period is scheduled to take place immediately.

Global settlement is subjected to popular vote like any CDP change or creation proposal, rather than designated to a select group of Keepers as in MakerDAO.

Global constants are the designated accounts that provide price feeds, the voting period of 2 weeks for proposals, and the minimum age of 5 minutes (how recent) for price data that is considered acceptable. 

###### TODO
medium.com/makerdao/dai-reward-rate-earn-a-reward-from-holding-dai-10a07f52f3cf