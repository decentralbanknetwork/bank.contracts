<h1 class="contract">bail</h1>
Reclaims collateral from an overcollateralized cdp, without taking the cdp below its liquidation ratio. 
<h1 class="contract">close</h1>
Close balance of given symbol for account.
<h1 class="contract">deposit</h1>
Description of the deposit function
<h1 class="contract">draw</h1>
Issue fresh stablecoin from this cdp.
<h1 class="contract">liquify</h1>
If the collateral value in a CDP drops below 150% of the outstanding stablecoin, the contract automatically sells enough of your collateral to buy back as many stablecoin as you issued. The issued stablecoin is thus taken out of circulation. Similar to a margin call.
<h1 class="contract">lock</h1>
Unless CDP is in liquidation, its owner can use lock to lock more collateral. 
<h1 class="contract">open</h1>
Open cdp or initiate balances for account.
<h1 class="contract">propose</h1>
Publish a proposal that either motions for the enactment of a new cdp type, or modification of an existing one.
<h1 class="contract">referended</h1>
Called either automatically after VOTE_PERIOD from the moment of proposal via "propose", via deferred action with proposer's auth, or (if deferred action fails) may be called directly by proposer after VOTE_PERIOD has passed. Proposal may be enacted if there are more votes in favor than against. Cleanup must be done regardless on relevant multi_index tables.
<h1 class="contract">settle</h1>
The system uses the market price of collateral at the time of settlement to compute how much collateral each user gets. 
<h1 class="contract">shut</h1>
Owner can close this CDP, if the price feed is up to date and the CDP is not in liquidation. Reclaims all collateral and cancels all issuance plus fee. 
<h1 class="contract">transfer</h1>
Transfer stablecoin
<h1 class="contract">upfeed</h1>
Price feed data update action.
<h1 class="contract">vote</h1>
Take a position (for/against) on a cdp proposal
<h1 class="contract">wipe</h1>
Owner can send back stablecoin and reduce the cdp's issued stablecoin balance.
<h1 class="contract">withdraw</h1>
Description of the withdraw function
