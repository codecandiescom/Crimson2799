P
@command~
{
  if(!StrICmp(CMD_CMD,"stop")) {
    BLOCK_CMD=TRUE;
    SendAction(CODE_THING,EVENT_THING,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST,"^a$N flashes out of existance.\n");
    ObjectStrip(EVENT_THING,4500,4599,BaseGetInside(EVENT_THING));
    ThingTo(EVENT_THING,WorldOf(164));
    SendAction(EVENT_THING,EVENT_THING,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST,"^a$N gets up out of a VR machine.\n");
    SendAction(EVENT_THING,EVENT_THING,SEND_DST|SEND_VISIBLE|SEND_CAPFIRST,"^aYour VR session has ended. You unhook yourself from a VR machine.\n");
    if(CharGetHitP(EVENT_THING)<CharGetHitPMax(EVENT_THING)>>1) {
      CharSetHitP(EVENT_THING,CharGetHitPMax(EVENT_THING)>>1);
    }
    CharAction(EVENT_THING,"look");
  }
  else {
    if(!StrICmp(CMD_CMD,"drop")) {
      BLOCK_CMD=TRUE;
      CharAction(EVENT_THING,COMMAND);
      ObjectStrip(BaseGetInside(EVENT_THING),0,4499,WorldOf(164));
      ObjectStrip(BaseGetInside(EVENT_THING),4600,2130706687,WorldOf(164));
    }
  }
}~
P
@death~
{
  if(ThingGetType(SPARE_THING)==TTYPE_PLR) {
    BLOCK_CMD=TRUE;
    SendAction(SPARE_THING,SPARE_THING,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST,"^a$N flashes out of existance.\n");
    ObjectStrip(SPARE_THING,4500,4599,BaseGetInside(SPARE_THING));
    ThingTo(SPARE_THING,WorldOf(164));
    SendAction(SPARE_THING,SPARE_THING,SEND_ROOM|SEND_VISIBLE|SEND_CAPFIRST,"^a$N gets up out of a VR machine... looking a little worse for wear.\n");
    SendAction(SPARE_THING,SPARE_THING,SEND_SRC|SEND_VISIBLE|SEND_CAPFIRST,"^aYour VR session has ended. You unhook yourself from a VR machine.\n");
    if(CharGetHitP(SPARE_THING)<CharGetHitPMax(SPARE_THING)>>1) {
      CharSetHitP(SPARE_THING,CharGetHitPMax(SPARE_THING)>>1);
    }
  }
}~
#0
SilverDale~
83 10 RESET-IMMEDIATELY|AREA-NO-TAKE
M 0 4500 10 4500 2             a Wolf Cub
M 0 4500 10 4500 2             a Wolf Cub
M 0 4500 10 4501 2             a Wolf Cub
M 0 4500 10 4501 2             a Wolf Cub
M 0 4500 10 4502 2             a Wolf Cub
M 0 4500 10 4502 2             a Wolf Cub
M 0 4501 10 4503 99            The Timberwolf
M 0 4500 10 4503 2             a Wolf Cub
M 0 4500 10 4503 2             a Wolf Cub
M 0 4500 10 4504 2             a Wolf Cub
M 0 4500 10 4504 2             a Wolf Cub
M 0 4500 10 4505 2             a Wolf Cub
M 0 4500 10 4505 2             a Wolf Cub
M 0 4501 8 4506 99             The Timberwolf
M 0 4501 8 4507 99             The Timberwolf
M 0 4501 8 4510 99             The Timberwolf
M 0 4501 8 4514 99             The Timberwolf
M 0 4502 4 4516 99             The Huge White Wolf
M 0 4501 8 4518 99             The Timberwolf
M 0 4502 4 4518 99             The Huge White Wolf
G 1 4519 2                       the key to SilverDale
O 0 4515 1 4521 1              a wooden keg
O 0 4516 1 4521 1              a coiled length of rope
M 0 4506 1 4522 99             The Vampire Mayor
G 1 4519 2                       the key to SilverDale
G 1 4505 2                       a pair of shadowgauntlets
G 1 4506 2                       a mageorb
M 0 4507 1 4522 99             The Enormous Shadow Wolf
D 0 4523 1 14                  By the East Gate-east
M 0 4502 4 4525 99             The Huge White Wolf
M 0 4501 8 4527 99             The Timberwolf
O 0 4517 1 4528 1              a grapple
O 0 4509 6 4529 1              a large silver ingot
M 0 4501 8 4530 99             The Timberwolf
M 0 4502 4 4533 99             The Huge White Wolf
M 0 4504 1 4534 99             The Vampire Orc-Shaman
G 1 4501 2                       a shadowblade
G 1 4507 2                       a priestorb
M 0 4505 6 4534 99             The Orc Bodyguard
M 0 4505 6 4534 99             The Orc Bodyguard
O 0 4510 1 4534 1              a large forge
M 0 4502 1 4535 99             The Huge White Wolf
M 0 4502 1 4535 99             The Huge White Wolf
O 0 4509 6 4535 1              a large silver ingot
D 0 4536 3 2                   Outside the East Gate-west
M 0 4503 13 4537 99            The Huge Shadow Wolf
M 0 4503 13 4540 99            The Huge Shadow Wolf
D 0 4542 1 2                   West of the Watch Tower-east
M 0 4503 13 4543 99            The Huge Shadow Wolf
M 0 4503 13 4546 99            The Huge Shadow Wolf
M 0 4503 13 4549 99            The Huge Shadow Wolf
M 0 4503 13 4551 99            The Huge Shadow Wolf
M 0 4503 13 4554 99            The Huge Shadow Wolf
M 0 4503 13 4557 99            The Huge Shadow Wolf
M 0 4503 13 4560 99            The Huge Shadow Wolf
M 0 4503 13 4563 99            The Huge Shadow Wolf
M 0 4503 13 4566 99            The Huge Shadow Wolf
M 0 4503 13 4569 99            The Huge Shadow Wolf
M 0 4503 13 4572 99            The Huge Shadow Wolf
D 0 4573 3 2                   Entranceway to the Watch Tower-west
M 0 4505 4 4582 99             The Orc Bodyguard
M 0 4505 4 4582 99             The Orc Bodyguard
M 0 4505 4 4582 99             The Orc Bodyguard
M 0 4505 4 4582 99             The Orc Bodyguard
M 0 4509 10 4584 99            The giant rat
M 0 4509 10 4584 99            The giant rat
M 0 4509 10 4584 99            The giant rat
M 0 4509 10 4584 99            The giant rat
M 0 4509 10 4584 99            The giant rat
M 0 4509 10 4584 99            The giant rat
M 0 4509 10 4584 99            The giant rat
M 0 4509 10 4584 99            The giant rat
M 0 4509 10 4584 99            The giant rat
M 0 4509 10 4584 99            The giant rat
M 0 4508 2 4585 99             A vampire mistress
E 1 4504 2                       a shadowbracer
M 0 4508 2 4585 99             A vampire mistress
E 1 4504 2                       a shadowbracer
M 0 4510 1 4586 99             Nosferatu
G 1 4520 1                       Vampire Heart
O 0 4514 14 4588 1             a wooden stake
O 0 4509 6 4591 1              a large silver ingot
O 0 4500 99 4591 1             a lantern
*
S
*
$
