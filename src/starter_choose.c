#include "gba/types.h"
#include "global.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "random.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "starter_choose.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_pokemon_sprites.h"
#include "trig.h"
#include "window.h"
#include "constants/songs.h"
#include "constants/rgb.h"
#include "constants/starter_choose.h"

#define STARTER_MON_COUNT 3

// Position of the sprite of the selected starter Pokémon
#define STARTER_PKMN_POS_X (DISPLAY_WIDTH / 2)
#define STARTER_PKMN_POS_Y 64

#define TAG_POKEBALL_SELECT 0x1000
#define TAG_STARTER_CIRCLE 0x1001

static void CB2_StarterChoose(void);
static void Task_StarterChoose(u8 taskId);
static void Task_HandleStarterChooseInput(u8 taskId);
static void Task_WaitForStarterSprite(u8 taskId);
static void Task_AskConfirmStarter(u8 taskId);
static void Task_HandleConfirmStarterInput(u8 taskId);
static void Task_DeclineStarter(u8 taskId);
static void Task_MoveStarterChooseCursor(u8 taskId);
static u8 CreatePokemonFrontSprite(u16 species, u8 x, u8 y);
static void SpriteCB_SelectionHand(struct Sprite *sprite);
static void SpriteCB_Pokeball(struct Sprite *sprite);
static void SpriteCB_StarterPokemon(struct Sprite *sprite);
static void GenerateStarterMons(void);

static u16 sStarterLabelWindowId;


EWRAM_DATA static struct Pokemon sStarterMons[STARTER_MON_COUNT] = {0};

const u16 gBirchBagGrass_Pal[] = INCBIN_U16("graphics/starter_choose/tiles.gbapal");
static const u16 sPokeballSelection_Pal[] = INCBIN_U16("graphics/starter_choose/pokeball_selection.gbapal");
static const u16 sStarterCircle_Pal[] = INCBIN_U16("graphics/starter_choose/starter_circle.gbapal");
const u32 gBirchBagTilemap[] = INCBIN_U32("graphics/starter_choose/birch_bag.bin.lz");
const u32 gBirchGrassTilemap[] = INCBIN_U32("graphics/starter_choose/birch_grass.bin.lz");
const u32 gBirchBagGrass_Gfx[] = INCBIN_U32("graphics/starter_choose/tiles.4bpp.lz");
const u32 gPokeballSelection_Gfx[] = INCBIN_U32("graphics/starter_choose/pokeball_selection.4bpp.lz");
static const u32 sStarterCircle_Gfx[] = INCBIN_U32("graphics/starter_choose/starter_circle.4bpp.lz");

static const struct WindowTemplate sWindowTemplates[] =
    {
        {.bg = 0,
         .tilemapLeft = 3,
         .tilemapTop = 15,
         .width = 24,
         .height = 4,
         .paletteNum = 14,
         .baseBlock = 0x0200},
        DUMMY_WIN_TEMPLATE,
};

static const struct WindowTemplate sWindowTemplate_ConfirmStarter =
    {
        .bg = 0,
        .tilemapLeft = 24,
        .tilemapTop = 9,
        .width = 5,
        .height = 4,
        .paletteNum = 14,
        .baseBlock = 0x0260};

static const u8 sPokeballCoords[STARTER_MON_COUNT][2] =
    {
        {60, 64},
        {120, 88},
        {180, 64},
};

static const u16 sStarterMon[REGION_COUNT][STARTER_MON_COUNT] = {
    [REGION_KANTO] =
        {
            SPECIES_BULBASAUR,
            SPECIES_CHARMANDER,
            SPECIES_SQUIRTLE},
    [REGION_JOHTO] =
        {
            SPECIES_CHIKORITA,
            SPECIES_CYNDAQUIL,
            SPECIES_TOTODILE},
    [REGION_HOENN] =
        {
            SPECIES_TREECKO,
            SPECIES_TORCHIC,
            SPECIES_MUDKIP,
        },
    [REGION_SINNOH] =
        {
            SPECIES_TURTWIG,
            SPECIES_CHIMCHAR,
            SPECIES_PIPLUP},
    [REGION_UNOVA] =
        {
            SPECIES_SNIVY,
            SPECIES_TEPIG,
            SPECIES_OSHAWOTT},
    [REGION_KALOS] =
        {
            SPECIES_CHESPIN,
            SPECIES_FENNEKIN,
            SPECIES_FROAKIE},
    [REGION_ALOLA] =
        {
            SPECIES_ROWLET,
            SPECIES_LITTEN,
            SPECIES_POPPLIO},
    [REGION_GALAR] =
        {
            SPECIES_GROOKEY,
            SPECIES_SCORBUNNY,
            SPECIES_SOBBLE},
    [REGION_PALDEA] =
        {
            SPECIES_SPRIGATITO,
            SPECIES_FUECOCO,
            SPECIES_QUAXLY},
};

static const struct BgTemplate sBgTemplates[3] =
    {
        {.bg = 0,
         .charBaseIndex = 2,
         .mapBaseIndex = 31,
         .screenSize = 0,
         .paletteMode = 0,
         .priority = 0,
         .baseTile = 0},
        {.bg = 2,
         .charBaseIndex = 0,
         .mapBaseIndex = 7,
         .screenSize = 0,
         .paletteMode = 0,
         .priority = 3,
         .baseTile = 0},
        {.bg = 3,
         .charBaseIndex = 0,
         .mapBaseIndex = 6,
         .screenSize = 0,
         .paletteMode = 0,
         .priority = 1,
         .baseTile = 0},
};

static const u8 sTextColors[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_LIGHT_GRAY};

static const struct OamData sOam_Hand =
    {
        .y = DISPLAY_HEIGHT,
        .affineMode = ST_OAM_AFFINE_OFF,
        .objMode = ST_OAM_OBJ_NORMAL,
        .mosaic = FALSE,
        .bpp = ST_OAM_4BPP,
        .shape = SPRITE_SHAPE(32x32),
        .x = 0,
        .matrixNum = 0,
        .size = SPRITE_SIZE(32x32),
        .tileNum = 0,
        .priority = 1,
        .paletteNum = 0,
        .affineParam = 0,
};

static const struct OamData sOam_Pokeball =
    {
        .y = DISPLAY_HEIGHT,
        .affineMode = ST_OAM_AFFINE_OFF,
        .objMode = ST_OAM_OBJ_NORMAL,
        .mosaic = FALSE,
        .bpp = ST_OAM_4BPP,
        .shape = SPRITE_SHAPE(32x32),
        .x = 0,
        .matrixNum = 0,
        .size = SPRITE_SIZE(32x32),
        .tileNum = 0,
        .priority = 1,
        .paletteNum = 0,
        .affineParam = 0,
};

static const struct OamData sOam_StarterCircle =
    {
        .y = DISPLAY_HEIGHT,
        .affineMode = ST_OAM_AFFINE_DOUBLE,
        .objMode = ST_OAM_OBJ_NORMAL,
        .mosaic = FALSE,
        .bpp = ST_OAM_4BPP,
        .shape = SPRITE_SHAPE(64x64),
        .x = 0,
        .matrixNum = 0,
        .size = SPRITE_SIZE(64x64),
        .tileNum = 0,
        .priority = 1,
        .paletteNum = 0,
        .affineParam = 0,
};

static const u8 sCursorCoords[][2] =
    {
        {60, 32},
        {120, 56},
        {180, 32},
};

static const union AnimCmd sAnim_Hand[] =
    {
        ANIMCMD_FRAME(48, 30),
        ANIMCMD_END,
    };

static const union AnimCmd sAnim_Pokeball_Still[] =
    {
        ANIMCMD_FRAME(0, 30),
        ANIMCMD_END,
    };

static const union AnimCmd sAnim_Pokeball_Moving[] =
    {
        ANIMCMD_FRAME(16, 4),
        ANIMCMD_FRAME(0, 4),
        ANIMCMD_FRAME(32, 4),
        ANIMCMD_FRAME(0, 4),
        ANIMCMD_FRAME(16, 4),
        ANIMCMD_FRAME(0, 4),
        ANIMCMD_FRAME(32, 4),
        ANIMCMD_FRAME(0, 4),
        ANIMCMD_FRAME(0, 32),
        ANIMCMD_FRAME(16, 8),
        ANIMCMD_FRAME(0, 8),
        ANIMCMD_FRAME(32, 8),
        ANIMCMD_FRAME(0, 8),
        ANIMCMD_FRAME(16, 8),
        ANIMCMD_FRAME(0, 8),
        ANIMCMD_FRAME(32, 8),
        ANIMCMD_FRAME(0, 8),
        ANIMCMD_JUMP(0),
    };

static const union AnimCmd sAnim_StarterCircle[] =
    {
        ANIMCMD_FRAME(0, 8),
        ANIMCMD_END,
    };

static const union AnimCmd *const sAnims_Hand[] =
    {
        sAnim_Hand,
    };

static const union AnimCmd *const sAnims_Pokeball[] =
    {
        sAnim_Pokeball_Still,
        sAnim_Pokeball_Moving,
    };

static const union AnimCmd *const sAnims_StarterCircle[] =
    {
        sAnim_StarterCircle,
    };

static const union AffineAnimCmd sAffineAnim_StarterPokemon[] =
    {
        AFFINEANIMCMD_FRAME(16, 16, 0, 0),
        AFFINEANIMCMD_FRAME(16, 16, 0, 15),
        AFFINEANIMCMD_END,
    };

static const union AffineAnimCmd sAffineAnim_StarterCircle[] =
    {
        AFFINEANIMCMD_FRAME(20, 20, 0, 0),
        AFFINEANIMCMD_FRAME(20, 20, 0, 15),
        AFFINEANIMCMD_END,
    };

static const union AffineAnimCmd *const sAffineAnims_StarterPokemon = {sAffineAnim_StarterPokemon};
static const union AffineAnimCmd *const sAffineAnims_StarterCircle[] = {sAffineAnim_StarterCircle};

static const struct CompressedSpriteSheet sSpriteSheet_PokeballSelect[] =
    {
        {.data = gPokeballSelection_Gfx,
         .size = 0x0800,
         .tag = TAG_POKEBALL_SELECT},
        {}};

static const struct CompressedSpriteSheet sSpriteSheet_StarterCircle[] =
    {
        {.data = sStarterCircle_Gfx,
         .size = 0x0800,
         .tag = TAG_STARTER_CIRCLE},
        {}};

static const struct SpritePalette sSpritePalettes_StarterChoose[] =
    {
        {.data = sPokeballSelection_Pal,
         .tag = TAG_POKEBALL_SELECT},
        {.data = sStarterCircle_Pal,
         .tag = TAG_STARTER_CIRCLE},
        {},
    };

static const struct SpriteTemplate sSpriteTemplate_Hand =
    {
        .tileTag = TAG_POKEBALL_SELECT,
        .paletteTag = TAG_POKEBALL_SELECT,
        .oam = &sOam_Hand,
        .anims = sAnims_Hand,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_SelectionHand};

static const struct SpriteTemplate sSpriteTemplate_Pokeball =
    {
        .tileTag = TAG_POKEBALL_SELECT,
        .paletteTag = TAG_POKEBALL_SELECT,
        .oam = &sOam_Pokeball,
        .anims = sAnims_Pokeball,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_Pokeball};

static const struct SpriteTemplate sSpriteTemplate_StarterCircle =
    {
        .tileTag = TAG_STARTER_CIRCLE,
        .paletteTag = TAG_STARTER_CIRCLE,
        .oam = &sOam_StarterCircle,
        .anims = sAnims_StarterCircle,
        .images = NULL,
        .affineAnims = sAffineAnims_StarterCircle,
        .callback = SpriteCB_StarterPokemon};

// .text
u16 GetStarterPokemon(u16 chosenStarterId)
{
    if (chosenStarterId > STARTER_MON_COUNT)
        chosenStarterId = 0;
    return sStarterMon[VarGet(VAR_REGION_CHOSEN)][chosenStarterId];
}

static void VblankCB_StarterChoose(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// Data for Task_StarterChoose
#define tStarterSelection data[0]
#define tPkmnSpriteId data[1]
#define tCircleSpriteId data[2]

// Data for sSpriteTemplate_Pokeball
#define sTaskId data[0]
#define sBallId data[1]

void CB2_ChooseStarter(void)
{
    u8 taskId;
    u8 spriteId;

    SetVBlankCallback(NULL);

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG3CNT, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);

    ChangeBgX(0, 0, BG_COORD_SET);
    ChangeBgY(0, 0, BG_COORD_SET);
    ChangeBgX(1, 0, BG_COORD_SET);
    ChangeBgY(1, 0, BG_COORD_SET);
    ChangeBgX(2, 0, BG_COORD_SET);
    ChangeBgY(2, 0, BG_COORD_SET);
    ChangeBgX(3, 0, BG_COORD_SET);
    ChangeBgY(3, 0, BG_COORD_SET);

    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);

    LZ77UnCompVram(gBirchBagGrass_Gfx, (void *)VRAM);
    LZ77UnCompVram(gBirchBagTilemap, (void *)(BG_SCREEN_ADDR(6)));
    LZ77UnCompVram(gBirchGrassTilemap, (void *)(BG_SCREEN_ADDR(7)));

    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    InitWindows(sWindowTemplates);

    DeactivateAllTextPrinters();
    LoadUserWindowBorderGfx(0, 0x2A8, BG_PLTT_ID(13));
    ClearScheduledBgCopiesToVram();
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetAllPicSprites();
    GenerateStarterMons();

    LoadPalette(GetOverworldTextboxPalettePtr(), BG_PLTT_ID(14), PLTT_SIZE_4BPP);
    LoadPalette(gBirchBagGrass_Pal, BG_PLTT_ID(0), sizeof(gBirchBagGrass_Pal));
    LoadCompressedSpriteSheet(&sSpriteSheet_PokeballSelect[0]);
    LoadCompressedSpriteSheet(&sSpriteSheet_StarterCircle[0]);
    LoadSpritePalettes(sSpritePalettes_StarterChoose);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);

    EnableInterrupts(DISPSTAT_VBLANK);
    SetVBlankCallback(VblankCB_StarterChoose);
    SetMainCallback2(CB2_StarterChoose);

    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN0_CLR);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_TGT1_BG2 | BLDCNT_TGT1_BG3 | BLDCNT_TGT1_OBJ | BLDCNT_TGT1_BD | BLDCNT_EFFECT_DARKEN);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 7);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);

    ShowBg(0);
    ShowBg(2);
    ShowBg(3);

    taskId = CreateTask(Task_StarterChoose, 0);
    gTasks[taskId].tStarterSelection = 1;

    // Create hand sprite
    spriteId = CreateSprite(&sSpriteTemplate_Hand, 120, 56, 2);
    gSprites[spriteId].data[0] = taskId;

    // Create three Poké Ball sprites
    spriteId = CreateSprite(&sSpriteTemplate_Pokeball, sPokeballCoords[0][0], sPokeballCoords[0][1], 2);
    gSprites[spriteId].sTaskId = taskId;
    gSprites[spriteId].sBallId = 0;

    spriteId = CreateSprite(&sSpriteTemplate_Pokeball, sPokeballCoords[1][0], sPokeballCoords[1][1], 2);
    gSprites[spriteId].sTaskId = taskId;
    gSprites[spriteId].sBallId = 1;

    spriteId = CreateSprite(&sSpriteTemplate_Pokeball, sPokeballCoords[2][0], sPokeballCoords[2][1], 2);
    gSprites[spriteId].sTaskId = taskId;
    gSprites[spriteId].sBallId = 2;

    sStarterLabelWindowId = WINDOW_NONE;
}

static void CB2_StarterChoose(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void Task_StarterChoose(u8 taskId)
{
    u8 spriteId;
    u8 selection = gTasks[taskId].tStarterSelection;

    DrawStdFrameWithCustomTileAndPalette(0, FALSE, 0x2A8, 0xD);
    AddTextPrinterParameterized(0, FONT_NORMAL, gText_BirchInTrouble, 0, 1, 0, NULL);
    PutWindowTilemap(0);
    ScheduleBgCopyTilemapToVram(0);

    // Create initial sprites for the selected starter
    spriteId = CreateSprite(&sSpriteTemplate_StarterCircle, sPokeballCoords[selection][0], sPokeballCoords[selection][1], 1);
    gTasks[taskId].tCircleSpriteId = spriteId;

    spriteId = CreatePokemonFrontSprite(GetStarterPokemon(selection), sPokeballCoords[selection][0], sPokeballCoords[selection][1]);
    gSprites[spriteId].affineAnims = &sAffineAnims_StarterPokemon;
    gSprites[spriteId].callback = SpriteCB_StarterPokemon;
    gTasks[taskId].tPkmnSpriteId = spriteId;

    gTasks[taskId].func = Task_HandleStarterChooseInput;
}

static void Task_HandleStarterChooseInput(u8 taskId)
{
    u8 selection = gTasks[taskId].tStarterSelection;

    if (JOY_NEW(DPAD_LEFT))
    {
        if (selection > 0)
            selection--;
        else
            selection = STARTER_MON_COUNT - 1;
        gTasks[taskId].tStarterSelection = selection;
        gTasks[taskId].func = Task_MoveStarterChooseCursor;
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        if (selection < STARTER_MON_COUNT - 1)
            selection++;
        else
            selection = 0;
        gTasks[taskId].tStarterSelection = selection;
        gTasks[taskId].func = Task_MoveStarterChooseCursor;
    }
    else if (JOY_NEW(A_BUTTON))
    {
        gTasks[taskId].func = Task_AskConfirmStarter;
    }
}

static void Task_WaitForStarterSprite(u8 taskId)
{
    if (gSprites[gTasks[taskId].tCircleSpriteId].affineAnimEnded &&
        gSprites[gTasks[taskId].tCircleSpriteId].x == STARTER_PKMN_POS_X &&
        gSprites[gTasks[taskId].tCircleSpriteId].y == STARTER_PKMN_POS_Y)
    {
        gTasks[taskId].func = Task_AskConfirmStarter;
    }
}

static void Task_AskConfirmStarter(u8 taskId)
{
    u16 species = GetStarterPokemon(gTasks[taskId].tStarterSelection);
    const u8 *speciesName = GetSpeciesName(species);
    
    PlayCry_Normal(species, 0);
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    StringCopy(gStringVar1, speciesName);
    StringExpandPlaceholders(gStringVar4, gText_ConfirmStarterChoice);
    AddTextPrinterParameterized(0, FONT_NORMAL, gStringVar4, 0, 1, 0, NULL);
    ScheduleBgCopyTilemapToVram(0);
    CreateYesNoMenu(&sWindowTemplate_ConfirmStarter, 0x2A8, 0xD, 0);
    gTasks[taskId].func = Task_HandleConfirmStarterInput;
}

static void Task_HandleConfirmStarterInput(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // YES
        // Give the pre-generated starter
        ZeroPlayerPartyMons();
        gPlayerParty[0] = sStarterMons[gTasks[taskId].tStarterSelection];
        gSpecialVar_Result = gTasks[taskId].tStarterSelection;
        ResetAllPicSprites();
        SetMainCallback2(gMain.savedCallback);
        break;
    case 1: // NO
    case MENU_B_PRESSED:
        PlaySE(SE_SELECT);
        // Restore original text
        FillWindowPixelBuffer(0, PIXEL_FILL(1));
        AddTextPrinterParameterized(0, FONT_NORMAL, gText_BirchInTrouble, 0, 1, 0, NULL);
        ScheduleBgCopyTilemapToVram(0);
        gTasks[taskId].func = Task_HandleStarterChooseInput;
        break;
    }
}

static void Task_DeclineStarter(u8 taskId)
{
    gTasks[taskId].func = Task_HandleStarterChooseInput;
}

static void Task_MoveStarterChooseCursor(u8 taskId)
{
    u8 spriteId;
    u8 selection = gTasks[taskId].tStarterSelection;

    // Clear previous Pokémon sprite if it exists
    if (gTasks[taskId].tPkmnSpriteId != 0)
    {
        spriteId = gTasks[taskId].tPkmnSpriteId;
        FreeOamMatrix(gSprites[spriteId].oam.matrixNum);
        FreeAndDestroyMonPicSprite(spriteId);
        gTasks[taskId].tPkmnSpriteId = 0;
    }

    // Clear previous circle sprite if it exists
    if (gTasks[taskId].tCircleSpriteId != 0)
    {
        spriteId = gTasks[taskId].tCircleSpriteId;
        FreeOamMatrix(gSprites[spriteId].oam.matrixNum);
        DestroySprite(&gSprites[spriteId]);
        gTasks[taskId].tCircleSpriteId = 0;
    }

    // Create new sprites for the selected starter
    spriteId = CreateSprite(&sSpriteTemplate_StarterCircle, sPokeballCoords[selection][0], sPokeballCoords[selection][1], 1);
    gTasks[taskId].tCircleSpriteId = spriteId;

    spriteId = CreatePokemonFrontSprite(GetStarterPokemon(selection), sPokeballCoords[selection][0], sPokeballCoords[selection][1]);
    gSprites[spriteId].affineAnims = &sAffineAnims_StarterPokemon;
    gSprites[spriteId].callback = SpriteCB_StarterPokemon;
    gTasks[taskId].tPkmnSpriteId = spriteId;

    gTasks[taskId].func = Task_HandleStarterChooseInput;
}

static u8 CreatePokemonFrontSprite(u16 species, u8 x, u8 y)
{
    u8 spriteId;
    u8 selection = gTasks[0].tStarterSelection;
    bool8 isShiny = IsMonShiny(&sStarterMons[selection]);

    spriteId = CreateMonPicSprite_Affine(species, isShiny, 0, MON_PIC_AFFINE_FRONT, x, y, 14, TAG_NONE);
    gSprites[spriteId].oam.priority = 0;
    return spriteId;
}

static void SpriteCB_SelectionHand(struct Sprite *sprite)
{
    // Float up and down above selected Poké Ball
    sprite->x = sCursorCoords[gTasks[sprite->data[0]].tStarterSelection][0];
    sprite->y = sCursorCoords[gTasks[sprite->data[0]].tStarterSelection][1];
    sprite->y2 = Sin(sprite->data[1], 8);
    sprite->data[1] = (u8)(sprite->data[1]) + 4;
}

static void SpriteCB_Pokeball(struct Sprite *sprite)
{
    // Animate Poké Ball if currently selected
    if (gTasks[sprite->sTaskId].tStarterSelection == sprite->sBallId)
        StartSpriteAnimIfDifferent(sprite, 1);
    else
        StartSpriteAnimIfDifferent(sprite, 0);
}

static void SpriteCB_StarterPokemon(struct Sprite *sprite)
{
    // Move sprite to upper center of screen
    if (sprite->x > STARTER_PKMN_POS_X)
        sprite->x -= 4;
    if (sprite->x < STARTER_PKMN_POS_X)
        sprite->x += 4;
    if (sprite->y > STARTER_PKMN_POS_Y)
        sprite->y -= 2;
    if (sprite->y < STARTER_PKMN_POS_Y)
        sprite->y += 2;
}

static void GenerateStarterMons(void)
{
    u16 species;
    u8 i;
    
    for (i = 0; i < STARTER_MON_COUNT; i++)
    {
        species = GetStarterPokemon(i);
        CreateMon(&sStarterMons[i], species, 5, 32, TRUE, Random32(), OT_ID_PLAYER_ID, 0);
    }
}
