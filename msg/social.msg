; Basic format
; #<social>
; <Position> or "SAME"
; <message type of N or O>
; source message~
; room message~
; 
; #<social>
; <Position> or "SAME"
; <message type of C>
; source message~
; dest message~
; room message~
;
; NOTE on "SAME", a position of same means RESTING if O or N, and the same 
; position as the target char in the case of C
;
; Unlike fight messages, each social command can have only a single entry
; for a given type ie only one N set, only one C set. If there is no set for
; a give type, then the social cannot be used against that kind of target.
; ie if there is only a C set, then the social must be directed at a character
; or it will complain when the player uses the command.
;
; message type can be:
; N - Sent if there is no target ie frown
; C - Sent if target is a character ie frown cryogen
; O - Sent if target is an object ie frown reactor
;
; For each social message, reserved char. sequences include:
; all SendAction sequences ($n, $N etc)
;
; NOTE: For Gods sake do yourself a favour and keep this file in alphabetical
; order.

#admire
RESTING
C
^PYou let your gaze linger admiringly on $N.~
^p$n lets $s gaze linger admiringly at you.~
^p$n lets $s gaze linger admiringly at $N.~

#beam
RESTING
N
^PYou beam with pride.~
^p$n beams with pride.~
C
^PYou beam with pride at $N.~
^p$n beams with pride at you.~
^p$n beams with pride at $N.~

#blush
RESTING
N
^PYou blush.~
^p$n blushes.~

#bounce
STANDING
N
^PYou bounce around happily!~
^p$n bounces around happily!~

#bow
STANDING
N
^PYou bow deeply.~
^p$n bows deeply.~
C
^PYou bow before $M.~
^p$n bows before you.~
^p$n bows before $N.~

#burp
RESTING
N
^PYou burp loudly.~
^p$n burps loudly.~

#cackle
RESTING
N
^PYou cackle gleefully.~
^p$n cackles with insane glee!~

#chuckle
RESTING
N
^PYou chuckle politely.~
^p$n chuckles politely.~

#comb
SITTING
N
^PYou comb your hair -- perfect.~
^p$n combs $s hair -- not a strand out place!~

#comfort
STANDING
C
^PYou comfort $N.~
^p$n comforts you.~
^p$n comforts $N.~

#cough
RESTING
N
^PYou cough loudly~
^p$n coughs loudly~

#cringe
STANDING
N
^PYou cringe in terror.~
^p$n cringes in terror!~
C
^PYou cringe away from $N.~
^p$n cringes away from you.~
^p$n cringes away from $N.~

#cry
RESTING
N
^PYou burst into tears.~
^p$n bursts into tears.~
C
^PYou cry on $N's shoulder.~
^p$n cries on your shoulder.~
^p$n cries on $N's shoulder.~

#cuddle
SITTING
C
^PYou cuddle $N lovingly.~
^p$n cuddles $N lovingly.~
^p$n cuddles you lovingly.~

#cuff
RESTING
C
^PYou give $N a quick cuff to the back of the head.~
^p$n gives you a quick cuff to the back of the head.~
^p$n gives $N a quick cuff to the back of the head.~

#dance
STANDING
N
^PYou skip and dance around by yourself.~
^p$n skips and dances around.~
C
^PYou grab $N and dance around.~
^p$n grabs you and dances around.~
^p$n grabs $N and dances around.~

#flip
STANDING
N
^PYou flip head over heels.~
^p$n flips head over heels.~

#fondle
SAME
C
^PYou fondle $N lovingly.~
^p$n fondles you lovingly.~
^p$n fondles $N lovingly.~

#frown
RESTING
N
^PYou frown with displeasure~
^p$n frowns with displeasure~
C
^PYou frown at $N~
^p$n frowns at you~
^p$n frowns at $N~

#fume
RESTING
N
^RYou grit your teeth and fume with rage.~
^r$n grits $s teeth and fumes with rage.~

#gasp
RESTING
N
^PYou gasp in astonishment.~
^p$n gasps in astonishment.~

#giggle
RESTING
N
^PYou giggle.~
^p$n giggles.~

#glare
RESTING
C
^PYou glare icily at $N.~
^p$n glares icily at you.~
^p$n glares at $N.~

#grimace
RESTING
N
^PYou screw up your face in a painfull grimace.~
^p$n screws up $s face in a painfull grimace.~

#grin
RESTING
N
^PYou grin evilly~
^p$n grins evilly~
C
^PYou flash $N an evil grin.~
^p$n flashes you an evil grin~
^p$n flashes $N an evil grin~

#groan
RESTING
N
^PYou groan loudly~
^p$n groans loudly~

#grope
SAME
C
^PYou grope $N fondly~
$n gropes $N fondly.~
$n gropes you fondly.~

#growl
RESTING
N
^PYou growl like a wild animal~
^p$n growls like a wild animal~

#grumble
RESTING
N
^PYou grumble good-naturedly~
^p$n grumbles good-naturedly~

#hate
RESTING
C
^PYou stare at $M, emitting a palpable aura of hatred.~
^p$n stares at you, emitting a palpable aura of hatred.~
^p$n stares at $N, emitting a palpable aura of hatred.~

#high
STANDING
C
^PYou leap up in the air and give $N a high-five.~
^p$n leaps up in the air and gives you a high-five.~
^p$n leaps up in the air and give $N a high-five.~

#howl
SITTING
N
^PYou throw your head back and howl at the top of your voice.~
^p$n throws back $s head and howls at the top of $s voice!~

#hug
SITTING
N
^PYou hug yourself excitedly~
^p$n hugs $mself excitedly~
C
^PYou give $N a friendly hug~
^p$n gives you a friendly hug~
^p$n gives $N a friendly hug~

#kiss
SAME
C
^PYou kiss $N passionately.~
^p$n kisses you passionately.~
^p$n kisses $N passionately.~

#laugh
RESTING
N
^PYou fall down laughing.~
^p$n falls down laughing.~

#lick
SAME
N
^PYou lick your lips and smile.~
^p$n licks $s lips and smiles.~
C
^PYou lick $N.~
^p$n licks you.~
^p$n licks $N.~

#moan
RESTING
N
^PYou start to moan.~
^p$n starts moaning.~

#mutter
RESTING
N
^PYou mutter curses under your breath.~
^p$n mutters curses under $s breath.~
C
^PYou mutter curses under your breath at $N.~
^p$n mutters curses under $s breath at you.~
^p$n mutters curses under $s breath at $N.~

#nibble
SAME
C
^PYou nibble on $N's ear.~
^p$n nibbles on your ear.~
^p$n nibbles on $N's ear.~

#nod
RESTING
N
^PYou nod solemnly~
^p$n nods solemnly~
C
^PYou nods towards $N~
^p$n nods towards you~
^p$n nods towards $N~

#nudge
SITTING
C
You nudge $M with your elbow.~
$n nudges you with $s elbow.~
$n nudges $N with $s elbow.~

#nuzzle
SAME
C
^PYou nuzzle $N's neck softly.~
^p$n softly nuzzles your neck.~
^p$n softly nuzzles $N's neck.~

#pat
STANDING
C
^PYou pat $N on the head~
^p$n pats you on the head~
^p$n pats $N on the head~

#peer
RESTING
C
^PYou peer suspiciously at $N~
^p$n peers suspiciously at you~
^p$n peers suspiciously at $N~

#point
RESTING
C
^PYou point at $N.~
^p$n points at $N.~
^p$n points at you.~
O
^PYou point at $N.~
^p$n points at $N.~

#poke
STANDING
C
^PYou poke $N in the ribs~
^p$n pokes you in the ribs~
^p$n pokes $N in the ribs~
O
^PYou gingerly poke the $N~
^p$n gingerly pokes the $N~

#ponder
RESTING
N
^PYou scratch your chin, and ponder for a moment~
^p$n scratches $s chin, and ponders for a moment~
C
^PYou ponder $N for a moment~
^p$n ponders you for moment~
^p$n ponders $N for a moment~

#pout
STANDING
N
^PYou slink to a corner and pout~
^p$n slinks into a corner and pouts~

#puke
STANDING
N
^PYou puke on the floor~
^p$n abruptly pukes on the floor~
C
^PYou puke right on $N's shoes~
^p$n pukes right on your shoes~
^p$n pukes right on $N's shoes~
O
^PYou pukes all over $N~
^p$n pukes all over $N~

#purr
RESTING
N
^PYou purr contentedly, just like a big tiger~
^p$n purr contentedly, just like a big tiger~

#raise
RESTING
N
^PYou raise one eyebrow sardonically.~
^p$n raises $s eyebrow sardonically.~

#roll
RESTING
N
^PYou roll your eyes in disgust.~
^p$n rolls $s eyes in disgust.~

#ruffle
STANDING
C
^PYou ruffle $N's hair playfully.~
^p$n ruffles your hair playfully.~
^p$n ruffles $N's hair playfully.~

#scowl
RESTING
N
^PYou scowl menacingly~
^p$n scowls menacingly~
C
^PYou scowl menacingly at $N~
^p$n scowls menacingly at you~
^p$n scowls menacingly at $N~

#scream
RESTING
N
^PYou let out a loud scream~
^p$n lets out a loud scream!~

#shake
SAME
N
^PYou shake your head.~
^p$n shakes $s head.~
C
^PYou shake $N's hand.~
^p$n shakes your hand.~
^p$n shakes $N's hand.~

#shiver
RESTING
N
^PYou shiver uncomfortably.~
^p$n shivers uncomfortably.~

#shooing
RESTING
N
^PYou make a vigorous shooing gesture with your hands~
^p$n makes a vigorous shooing gesture with $s hands~
C
^PYou makes a vigorous shooing gesture at $N~
^p$n makes a vigorous shooing gesture at you~
^p$n makes a vigorous shooing gesture at $N~

#shrug
RESTING
N
^PYou shrug helplessly.~
^p$n shrugs helplessly.~

#shush
RESTING
N
^PYou make a noisy shushing sound~
^p$n makes a noisy shushing sound~
C
^PYou noisily shush $N~
^p$n noisily shushes you~
^p$n noisily shushes $N~

#sigh
RESTING
N
^PYou sigh loudly.~
^p$n sighs loudly.~

#slap
RESTING
N
^PYou slap yourself, silly you~
^p$n abruptly slaps $mself~
C
^PYou slap $N across the face.~
^p$n slaps you across the face.~
^p$n slaps $N across the face.~

#smile
RESTING
N
^PYou smile cheerfully~
^p$n smiles cheerfully~
C
^PYou beam a cheery smile at $N~
^p$n beams a cheery smile your way~
^p$n beams a cheery smile at $N~

#smirk
RESTING
N
^PYou smirk knowingly~
^p$n smirks knowingly~
C
^PYou smirk knowingly at $N~
^p$n smirks knowingly at you~
^p$n smirks knowingly at $N~

#snap
RESTING
N
^PYou snap your fingers.~
^p$n snaps $s fingers.~

#snarl
RESTING
N
You snarl angrily.~
$n snarls angrily.~
C
You snarl angrily at $N.~
$n snarls angrily at you.~
$n snarls angrily at $N.~

#sneer
RESTING
N
You curl your lip and sneer disdainfully.~
$n curls $s lip and sneers disdainfully.~
C
You curl your lip and sneer disdainfully at $N.~
$n curls $s lip and sneers disdainfully at you.~
$n curls $s lip and sneers disdainfully at $N.~

#snicker
RESTING
N
^PYou snicker softly.~
^p$n snickers softly.~

#sniff
RESTING
N
^PYou sniff sadly.~
^p$n sniffs sadly.~

#snuggle
SAME
C
^PYou snuggle up to $N.~
^p$n snuggles up to you.~
^p$n snuggles up to $N.~

#sob
RESTING
N
^PYou sob quietly.~
^p$n sobs quietly.~

#squeeze
SITTING
C
^PYou give $N a friendly squeeze on the arm.~
^p$n gives you a friendly squeeze on the arm.~
^p$n gives $N a friendly squeeze on the arm.~

#strut
STANDING
N
^PYou strut proudly.~
^p$n struts proudly.~

#sulk
RESTING
N
^PYou sulk in a corner.~
^p$n sulks in the corner.~

#swear
RESTING
N
^RYou swear loudly: #@*"*&^$$%@*&!!!!!!~
^r$n swears loudly: #@*"*&^$$%@*&!!!!!!~
C
^RYou swear loudly at $n~
^r$n swears loudly at you~
^r$n swears loudly at $N~

#sweat
RESTING
N
^PYou suddenly break out into a cold sweat.~
^p$n suddenly breaks out into a cold sweat.~

#taunt
STANDING
C
^PYou taunt $N rather insultingly.~
^p$n taunts $N rather insultingly.~
^p$n taunts you rather insultingly.~

#thank
RESTING
C
^PYou thank $N heartily.~
^p$n thanks you heartily.~
^p$n thanks $N heartily.~

#tickle
SAME
C
^PYou tickle $N.~
^p$n tickles you.~
^p$n tickles $N.~

#twiddle
RESTING
N
^PYou patiently twiddle your thumbs.~
^p$n patiently twiddles $s thumbs.~

#waggle
RESTING
N
^PYou waggle your eyebrows.~
^p$n waggles $s eyebrows.~
C
^PYou waggle your eyebrows at $N.~
^p$n waggles $s eyebrows at $N.~
^p$n waggles $s eyebrows at you.~

#wave
RESTING
N
^PYou wave happily.~
^p$n waves happily.~
C
^PYou wave goodbye to $N.~
^p$n waves goodbye to $N.~
^p$n waves goodbye to you.~

#whistle
RESTING
N
^PYou whistle appreciatively.~
^p$n whistles appreciatively.~

#wink
RESTING
N
^PYou wink suggestively.~
^p$n winks suggestively.~
C
^PYou wink suggestively at $N.~
^p$n winks suggestively at you.~
^p$n winks suggestively at $N.~

#wipe
RESTING
N
^PYou wipe the sweat from your brow in relief.~
^p$n wipes the sweat from $s brow in relief.~

#worship
STANDING
C
^PYou fall to your knees and worship $N.~
^p$n kneels before you in solemn worship.~
^p$n falls to $s knees, and worships $N.~

#yawn
RESTING
N
^PYou yawn noisily.~
^p$n yawns noisily.~

$
