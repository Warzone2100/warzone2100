/*
 * PowerCrypt.h
 *
 * Set up a seperate encrypted copy of each players power.
 *
 */
#ifndef _powercrypt_h

// set the current power value
void pwrcSetPlayerCryptPower(UDWORD player, UDWORD power);
// check the current power value
BOOL pwrcCheckPlayerCryptPower(UDWORD player, UDWORD power);
// check the valid flags and scream if the power isn't valid
void pwrcUpdate(void);

#endif


