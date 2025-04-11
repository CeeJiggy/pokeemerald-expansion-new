#ifndef GUARD_CONSTANTS_FIELD_SPECIALS_H
#define GUARD_CONSTANTS_FIELD_SPECIALS_H

// PC Locations
#define PC_LOCATION_OTHER           0
#define PC_LOCATION_BRENDANS_HOUSE  1
#define PC_LOCATION_MAYS_HOUSE      2

// SS Tidal Locations
#define SS_TIDAL_LOCATION_CURRENTS  0
#define SS_TIDAL_LOCATION_SLATEPORT 1
#define SS_TIDAL_LOCATION_LILYCOVE  2
#define SS_TIDAL_LOCATION_ROUTE124  3
#define SS_TIDAL_LOCATION_ROUTE131  4

#define SS_TIDAL_BOARD_SLATEPORT     1
#define SS_TIDAL_DEPART_SLATEPORT    2
#define SS_TIDAL_HALFWAY_LILYCOVE    3
#define SS_TIDAL_LAND_LILYCOVE       4
#define SS_TIDAL_BOARD_LILYCOVE      5
#define SS_TIDAL_DEPART_LILYCOVE     6
#define SS_TIDAL_HALFWAY_SLATEPORT   7
#define SS_TIDAL_LAND_SLATEPORT      8
#define SS_TIDAL_EXIT_CURRENTS_RIGHT 9
#define SS_TIDAL_EXIT_CURRENTS_LEFT  10

#define SS_TIDAL_MAX_STEPS 205

// Scrollable Multichoice Menus
#define SCROLL_MULTI_NONE                                  0
#define SCROLL_MULTI_GLASS_WORKSHOP_VENDOR                 1
#define SCROLL_MULTI_POKEMON_FAN_CLUB_RATER                2
#define SCROLL_MULTI_BF_EXCHANGE_CORNER_DECOR_VENDOR_1     3
#define SCROLL_MULTI_BF_EXCHANGE_CORNER_DECOR_VENDOR_2     4
#define SCROLL_MULTI_BF_EXCHANGE_CORNER_VITAMIN_VENDOR     5
#define SCROLL_MULTI_BF_EXCHANGE_CORNER_HOLD_ITEM_VENDOR   6
#define SCROLL_MULTI_BERRY_POWDER_VENDOR                   7
#define SCROLL_MULTI_BF_RECEPTIONIST                       8
#define SCROLL_MULTI_BF_MOVE_TUTOR_1                       9
#define SCROLL_MULTI_BF_MOVE_TUTOR_2                      10
#define SCROLL_MULTI_SS_TIDAL_DESTINATION                 11
#define SCROLL_MULTI_BATTLE_TENT_RULES                    12

#define MAX_SCROLL_MULTI_ON_SCREEN 6
#define MAX_SCROLL_MULTI_LENGTH 16

// Dept Store Floor Numbers
#define DEPT_STORE_FLOORNUM_B4F       0
#define DEPT_STORE_FLOORNUM_B3F       1
#define DEPT_STORE_FLOORNUM_B2F       2
#define DEPT_STORE_FLOORNUM_B1F       3
#define DEPT_STORE_FLOORNUM_1F        4
#define DEPT_STORE_FLOORNUM_2F        5
#define DEPT_STORE_FLOORNUM_3F        6
#define DEPT_STORE_FLOORNUM_4F        7
#define DEPT_STORE_FLOORNUM_5F        8
#define DEPT_STORE_FLOORNUM_6F        9
#define DEPT_STORE_FLOORNUM_7F       10
#define DEPT_STORE_FLOORNUM_8F       11
#define DEPT_STORE_FLOORNUM_9F       12
#define DEPT_STORE_FLOORNUM_10F      13
#define DEPT_STORE_FLOORNUM_11F      14
#define DEPT_STORE_FLOORNUM_ROOFTOP  15

// Lilycove Pokémon Trainer Fan Club
#define NUM_TRAINER_FAN_CLUB_MEMBERS  8

#define FANCLUB_GOT_FIRST_FANS 7
#define FANCLUB_MEMBER1        8
#define FANCLUB_MEMBER2        9
#define FANCLUB_MEMBER3        10
#define FANCLUB_MEMBER4        11
#define FANCLUB_MEMBER5        12
#define FANCLUB_MEMBER6        13
#define FANCLUB_MEMBER7        14
#define FANCLUB_MEMBER8        15

#define FANCOUNTER_DEFEATED_DRAKE    0
#define FANCOUNTER_BATTLED_AT_BASE   1
#define FANCOUNTER_FINISHED_CONTEST  2
#define FANCOUNTER_USED_BATTLE_TOWER 3

#define USED_FLY_WHISTLE 255 // used for determining if Fly or a flying taxi was used

// Return values for DoDeoxysRockInteraction
#define DEOXYS_ROCK_FAILED     0
#define DEOXYS_ROCK_PROGRESSED 1
#define DEOXYS_ROCK_SOLVED     2
#define DEOXYS_ROCK_COMPLETE   3

enum {
    OPEN_PARTY_SCREEN,
    NO_PARTY_SCREEN
};

enum {
    CURRENT_POSITION,
    TEMPLATE_POSITION
};

#endif // GUARD_CONSTANTS_FIELD_SPECIALS_H
