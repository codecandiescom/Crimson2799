; Basic format
; #<space separated list of weapon & effect type names>
; <message type>
; source message~
; dest message~
; room message~
;
; of course a type list can be followed by any number of message sets
; message type can be:
; 0 - miss message
; S - single hit message
; M - multiple hit message
; D - death message
;
; List of damage messages, reserved char include:
; all SendAction sequences ($n, $N etc) plus
; #h the number of hits, #d damage string (tickle, badly hurt, obliterate)
; #g ing damage string (tickling, hurting, obliterating), #p points of damage
; and #n for weapon name/effectname
; NOTE: damage string is related to percent damage inflicted of current hp

#MELEE-KNIFE
0
^wYou try to stab $N but miss badly~
^R$n tries to stab you but misses badly~
^R$n tries to stab $N but misses badly~
S
^wYou #d $N with a quick stab of your #n~
^r$n #ds you with a quick stab of $h #n~
^R$n #ds $N with a quick stab of $h #n~
M
^wYou #d $N with #h quick stabs of your #n~
^r$n #ds you with #h quick stabs of $h #n~
^R$n #ds $N with #h quick stabs of $h #n~
D
^wYou sink your #n up to the hilt in poor $N~
^r$n sinks $h #n into your chest... fade to black...~
^R$n sinks $h #n up to the hilt in poor $N~

#MELEE-BLADE MELEE-AXE
0
^wYou try to slash $N with your #n but miss badly~
^R$n tries to slash you with $h #n but misses badly~
^R$n tries to slash $N with $h #n but misses badly~
S
^wYou #d $N with a quick slash of your #n~
^r$n #ds you with a quick slash of $h #n~
^R$n #ds $N with a quick slash of $h #n~
M
^wYou #d $N with #h quick slashes of your #n~
^r$n #ds you with #h quick slashes of $h #n~
^R$n #ds $N with #h quick slashes of $h #n~
D
^wYou slash $N into two pieces... both dead~
^r$n slashes you in two~
^R$n slashes $N into two pieces... both dead~

#MELEE-BLUDGEON
0
^wYou try to bludgeon $N with your #n but miss badly~
^R$n tries to bludgeon you with $h #n but misses badly~
^R$n tries to bludgeon $N with $h #n but misses badly~
S
^wYou #d $N with a quick smash of your #n~
^r$n #ds you with a quick smash of $h #n~
^R$n #ds $N with a quick smash of $h #n~
M
^wYou #d $N with #h quick blows of your #n~
^r$n #ds you with #h quick blows of $h #n~
^R$n #ds $N with #h quick blows of $h #n~
D
^wYou ruthlessly pulverize $N with one final blow~
^r$n ruthlessly pulverizes you with one final blow~
^R$n ruthlessly pulverizes $N with one final blow~
