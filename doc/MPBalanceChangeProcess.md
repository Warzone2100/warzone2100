# Multiplayer Balance Changes - Proposal Process

## Phase 0:

Generate and discuss your idea for a balance change. There are many great places to do this and get some initial feedback, like the [project’s Discord](https://wz2100.net/webchat/) `#mp-balance` or `#suggestions` channels.

## Phase 1:
### Packaging a Proposal (on GitHub)

Once you have an idea for a proposed balance change, you can open a new PR on GitHub (on the [Warzone 2100 repo](https://github.com/Warzone2100/warzone2100)), making changes to the specific mp/stat(s) files.

**General Guidelines:**
- Proposals should be focused and limited in scope, to allow contributors to better assess and discuss the impact of a change.
- Please include:
    - Rationale and reasoning for the change
    - Any additional contributors / authors of the change (if applicable)
    - Links to any previous discussion of the change (such as on Discord or the forums)
    - An explanation of what games modes + tech modes + play styles / maps it has been tested on
    - Any additional comparisons, videos, spreadsheets / charts, or information that will aide contributors & players in assessing the impact of the change on balance and gameplay. (If you were very technical in coming up with how to calculate a change, feel free to include your work! People really like to be able to review the details.)
- No single mode or type of play should be advantaged _at the expense_ of other modes of play. (For example, competitive high-oil multiplayer at the expense of low-oil skirmish.) There are many ways to play and enjoy the game, and balance proposals that initially break one should be - at a minimum - adjusted with that feedback taken into account.

Initial discussion and analysis can then occur on GitHub.

If a proposed balance change PR reaches a stage where it is a candidate for possible inclusion (and isn’t ruled out for some reason, such as overtly breaking the game, being too big of a change, or lacking any explanation), it may continue to Phase 2.

## Phase 2:
### Discussion and Voting (on Discord)

To include a greater portion of the player-base and community (and increase visibility), discussion and voting should then be solicited via Discord:

- A member of the team should open a thread on the Warzone 2100 Discord’s [#mp-balance-proposals](https://discord.com/channels/684098359874814041/1093556039418400879) channel to gather more discussion and feedback from the player community.
   - Apply the tag `Step 1: Discussion`.
- The thread should link to the GitHub PR, and include a descriptive title for the change.
- All members of the `@multplayer-balance-tester` role should be pinged for discussion and voting
- Post a link to the thread back in the GitHub PR, and add the `mp-balance: phase 2` tag to the PR.

Discussion of the proposed balance change should then occur on Discord.

> ### Discussion Rules:
> - Discussion should be _constructive_, and is intended to:
>    - Discuss support or opposition to a proposal
>    - Build towards an understanding of any concerns with a proposal (and ideas to address them)
>    - Discuss alternatives that might better address the original rationale that led to the proposal
> - **Name-calling, personal attacks, or other abuse will not be tolerated**, and will result in:
>    - The agitator's vote **not counting**
>    - Possible removal of the `@multiplayer-balance-tester` role (and thus the ability to vote on future proposals)
>    - Other possible moderator actions

Once discussion seems to have reached a reasonable point for voting:
- A pinned post in the Discord thread should be created requesting reaction votes (thumbs up, thumbs down) (pinging `@multiplayer-balance-tester` again).
   - Change the thread tag to `Step 2: Voting`.
- Every person voting should reply to the call-to-vote message with their reasoning / explanation for their vote.
   - (To count, votes _must_ be accompanied by a post describing one’s reasoning. If there is any confusion, start your explanation with: **REASONS FOR MY VOTE:**)

A period of at least several days should be allowed for voting, to ensure maximum opportunity to engage.

If there is a greater than 75% consensus among qualified voters, and a sufficient number of qualified voters voting, a proposal _may_ proceed to Phase 3. (If not, the results should be posted to GitHub and changes should be requested, taking feedback into account - for example: trying a smaller & more incremental change to allow players to better adjust, or providing a more detailed explanation of the change and its impacts / desirability / need.)

## Phase 3:
### Merging (on GitHub)

Once a proposal has reached Phase 3, the PR _may_ be merged into the master branch, for inclusion in the default multiplayer balance for a future release.

When merging, please post a comment in the corresponding balance PR following this template:

```markdown
This balance proposal has progressed to Phase 3, and will be merged into the master branch.
<or>
This balance proposal has been closed without merging.

Phase 2 Discussion occurred at: <link to Discord thread>

Voting results were:
   - **Total Number of Votes**: <total number of qualified votes>
   - **Percentage Supporting**: <percentage in favor>%
   - Votes For: <list of players voting in favor>
   - Votes Against: <list of players voting against>
```
