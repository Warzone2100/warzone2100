
// A fallback build order for the standard ruleset.

function buildOrder_StandardFallback() {
	var derrickCount = countFinishedStructList(structures.derricks);
	// might be good for Insane AI, or for rebuilding
	if (derrickCount > 0)
		if (buildMinimum(structures.gens, 1)) return true;
	// lab, factory, gen, cc - the current trivial build order for the 3.2+ starting conditions
	if (buildMinimum(structures.labs, 1)) return true;
	if (buildMinimum(structures.factories, 1)) return true;
	if (buildMinimum(structures.gens, 1)) return true;
	// make sure trucks go capture some oil at this moment
	if (buildMinimumDerricks(1)) return true;
	// what if one of them is being upgraded? will need the other anyway.
	// also, it looks like the right timing in most cases.
	if (buildMinimum(structures.gens, 2)) return true;
	if (buildMinimum(structures.hqs, 1)) return true;
	// make sure we have at least that much oils by now
	if (buildMinimumDerricks(5)) return true;
	// support hover maps
	var ret = scopeRatings();
	if (ret.land === 0 && !iHaveHover())
		if (buildMinimum(structures.labs, 4)) return true;
	if (ret.land === 0 && ret.sea === 0 && !iHaveVtol())
		if (buildMinimum(structures.labs, 4)) return true;
	if (gameTime > 300000) {
		// build more factories and labs when we have enough income
		if (buildMinimum(structures.labs, derrickCount / 3)) return true;
		if (needFastestResearch() === PROPULSIONUSAGE.GROUND) {
			if (buildMinimum(structures.factories, 2)) return true;
			if (scopeRatings().land > 0)
				if (buildMinimum(structures.templateFactories, 1)) return true;
		}
		if (buildMinimum(structures.vtolFactories, 1)) return true;
		return false;
	}
	// support hover maps
	var ret = scopeRatings();
	if (ret.land === 0 && !iHaveHover())
		if (buildMinimum(structures.labs, 4)) return true;
	if (ret.land === 0 && ret.sea === 0 && !iHaveVtol())
		if (buildMinimum(structures.labs, 4)) return true;
	return true;
}
