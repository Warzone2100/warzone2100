debugMsg('Module: chatting.js', 'init');

var _chatting = {};

function chatting(type)
{
	const chlen = _chatting[type].length; // Забавно, спустя некотое время я только осознал, что за имя я дал этой переменной xDDD

	return _chatting[type][Math.floor(Math.random() * chlen)];
}

_chatting['army'] = [
	"If u want, I may support you with my army. Just say \"yes\"",
	"I can share some units, say \"yes\" if u want.",
	"I can sometimes give you my warriors, just say \"yes\""
];

_chatting['lassat_fire'] = [
	"Bada-Boom!",
	"Boom!",
	"Boo-Boom!",
	";)"
];

_chatting['lassat_charged'] = [
	"Scare me",
	"Laser ready, now get ready",
	"He-heh",
	":)"
];

_chatting['confirm'] = [
	"Ok then! Let's kill them all together!!",
	"Yeah! That's my boy! >:)",
	"Allright! Got it!",
	"Well, ok then ;)",
	"Ok, got it",
	"Ok"
];

_chatting['kick'] = [
	"I was kicked out, for what?",
	"Lol, just kicked..",
	"Well, bye, I leave.",
	"Losers, you're just afraid of me."
];

_chatting['saved'] = [
	"Yeah, better to be saved ;)",
	"Oh, what, decided to take a break?",
	"Save, save, save.. How long.",
	"Now you just put my brains in a box.",
	"Save me, save me again!",
	"Save your self, save your life!",
	"Oh, don't be scared.. Saved, nice :)"
];

_chatting['tutorial'] = [
	"By the way, you can ask me for money, just say \"bc give money\"",
	"If you die, just say \"bc give truck\", and i resurrect u ;)",
	"Btw, you can ask me, \"bc give money\" or \"bc give truck\" remember that",
	"If u don't know, team chat binding on Ctrl+Enter keys %)",
	"btw, u can fast rotate camera by pressing Ctrl+arrows ;)",
	"Always help the team, or we will all die"
];

_chatting['ally'] = [
	"Hellow my friend! Let's kill them!",
];

_chatting['threat'] = [
	'I kill you!!!',
	'Nice try! But I will destroy you!',
	'Do not expect mercy'
];

_chatting['welcome'] = [
	'Hello everyone',
	'Hi there',
	'hi.. gl hf',
	'gl',
	'glhf',
	'gl hf',
	'Hi!! ))',
	'gl ;)',
	'gl hf )',
	'Hello, good luck!',
	"I'm noob.. :|",
	'Hi all',
	'GLHF!',
	'have fun..',
	'yo',
	'hello',
	'=)',
	'yoyo, hi',
	'hi, how are you?',
	"приветы",
	"Здравствуйте",
	"Всем привет",
	"Dosvidaniya... no? %) Hi!"
];

_chatting['lose'] = [
	'ohh.. you win, gg',
	'nice, gg',
	'wow.. gg',
	"gg I'm loser",
	'ggwp',
	'gg wp',
	'sorry I am noob',
	'aaarrgghghh!! You are cheater!',
	'cheater? .. I lost..',
	'=( gg',
	'bye',
	'oh no! =( bye..',
	'How dare you!',
	'wtf?.. lol, gg',
	'nice cheat man.. bye'
];

_chatting['berserk'] = [
	'Time to kick someone\'s ..',
	'You made me angry',
	'You\'re clearly pissing me off',
	'You just got a problem',
	'I have a great deal on oil, and what\'s your trump card?',
	'Who needs oil. Oil for suckers'
];

_chatting['seer'] = [
	'You\'re a leather bag of brains, and I\'m just a cheater.',
	'Leather bag, aren\'t you afraid?',
	'I can see everything.',
	'Tons of oil - check, see the entire map - check.'
];

_chatting['no'] = [
	'I\'m sorry, I can\'t do this.',
	'No, I\'m not ready for that',
	'Sorry, but no.',
	'Unfortunately, no',
	'No, no, and no again',
	'No',
	'Nope',
	'I would love to, but no.',
	'Well, whatever you say.. but.. wait, no!'
];

_chatting['dev'] = ['This is dev version, dont use it! Данная версия бота не является релизом и может содержать баги! Не используйте её!'];
