// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (c) 2011-2015, 2019, Antonio Niño Díaz
//
// GiiBiiAdvance - GBA/GB emulator

#include "../build_options.h"

#include "gameboy.h"

typedef struct {
    const char code[2]; // An hexadecimal value would work for most codes, but some of them
    const char * name;  // (new licensees only) use other ascii characters.
} _licensee_t;

static const _licensee_t gb_licensee[] = {
    {"00","None"},
    {"01","Nintendo"},
    {"08","Capcom"},
    {"09","HOT.B Co., Ltd"},
    {"0A","Jaleco"},
    {"0B","Coconuts"},
    {"0C","Elite Systems/Coconuts"}, //?
    {"13","Electronic Arts"},
    {"18","Hudson soft"},
    {"19","B-AI/ITC Entertainment"}, //?
    {"1A","Yanoman"},
    {"1D","Clary"}, //?
    {"1F","Virgin Games/7-UP"}, //?
    {"20","KSS"}, //?
    {"22","Pow"}, //?
    {"24","PCM Complete"}, //?
    {"25","San-X"}, //?
    {"28","Kotobuki/Walt disney/Kemco Japan"}, //?
    {"29","Seta"},
    {"2L","Tamsoft"}, //?
    {"30","Infogrames/Viacom"}, //?
    {"31","Nintendo"}, //?
    {"32","Bandai"}, //?
    {"33","Ocean/Acclaim"}, //?
    {"34","Konami"}, //?
    {"35","Hector"},
    {"37","Taito"}, //?
    {"38","Kotobuki systems/Interactive TV ent/Capcom/Hudson"}, //?
    {"39","Telstar/Accolade/Banpresto"}, //?
    {"3C","Entertainment int./Empire/Twilight"}, //?
    {"3E","Gremlin Graphics/Chup Chup"}, //?
    {"41","UBI Soft"}, //?
    {"42","Atlus"}, //?
    {"44","Malibu"}, //?
    {"47","Spectrum Holobyte/Bullet Proof"}, //?
    {"46","Angel"}, //?
    {"49","Irem"},
    {"4A","Virgin"}, //?
    {"4D","Malibu (T-HQ)"},
    {"4F","U.S. Gold"},
    {"4J","Fox Interactive/Probe"}, //?
    {"4K","Time Warner"}, //?
    {"4S","Black Pearl (T-HQ)"},
    {"50","Absolute Entertainment"},
    {"51","Acclaim"},
    {"52","Activision"},
    {"53","American Sammy"}, //?
    {"54","Gametek/Infogenius Systems/Konami"}, //?
    {"55","Hi Tech Expressions/Park Place"},
    {"56","LJN. Toys Ltd"},
    {"57","Matchbox International"}, //?
    {"58","Mattel"}, //?
    {"59","Milton Bradley"}, //?
    {"5A","Mindscape"},
    {"5B","ROMStar"},
    {"5C","Taxan/Naxat soft"},
    {"5D","Williams/Tradewest/Rare"}, //?
    {"60","Titus"},
    {"61","Virgin Interactive"},
    {"64","LucasArts"}, //?
    {"67","Ocean"},
    {"69","Electronic Arts"},
    {"6E","Elite Systems"},
    {"6F","Electro brain"},
    {"70","Infogrames"},
    {"71","Interplay productions"},
    {"72","First Star Software/Broderbund"}, //?
    {"73","Sculptured software"}, //?
    {"75","The sales curve Ltd./SCi Entertainment Group"}, //?
    {"78","T-HQ Inc."},
    {"79","Accolade"},
    {"7A","Triffix Ent. Inc."}, //?
    {"7C","MicroProse/NMS"}, //?
    {"7D","Universal Interactive Studios"}, //?
    {"7F","Kotobuki Systems/Kemco"}, //?
    {"80","Misawa/NMS"}, //?
    {"83","LOZC/G.Amusements"}, //?
    {"86","Zener Works/Tokuna Shoten"}, //?
    {"87","Tsukuda Original"}, //?
    {"8B","Bullet-Proof software"},
    {"8C","Vic Tokai"},
    {"8E","Character soft/Sanrio/APE"}, //?
    {"8F","I'Max"},
    {"8M","CyberFront/Taito"}, //?
    {"91","Chun Soft"}, //?
    {"92","Video System"}, //?
    {"93","Bec/Tsuburava/Ocean/Acclaim"}, //?
    {"95","Varie"},
    {"96","Yonesawa/S'Pal"}, //?
    {"97","Kaneko"}, //?
    {"99","Pack-In-Video/ARC"},
    {"9A","Nihon Bussan"}, //?
    {"9B","Tecmo"},
    {"9C","Imagineer Co., Ltd"},
    {"9D","Banpresto"}, //?
    {"9F","Namco/Nova"}, //?
    {"A1","Hori electric"}, //?
    {"A2","Bandai"}, //?
    {"A4","Konami"},
    {"A6","Kawada"}, //?
    {"A7","Takara"},
    {"A9","Technos japan Corp."},
    {"AA","First Star software/Broderbund/dB soft"}, //?
    {"AC","Toei"},
    {"AD","TOHO Co., Ltd."}, //?
    {"AF","Namco hometek"},
    {"AG","Playmates/Shiny"}, //?
    {"AL","MediaFactory"}, //?
    {"B0","Acclaim/LJN"}, //?
    {"B1","Nexoft/ASCII"}, //?
    {"B2","Bandai"},
    {"B4","Enix"},
    {"B6","HAL"},
    {"B7","SNK (america)"},
    {"B9","Pony Canyon"},
    {"BA","Culture Brain"},
    {"BB","SunSoft"},
    {"BD","Sony Imagesoft"},
    {"BF","Sammy"},
    {"BL","MTO Inc."}, //?
    {"C0","Taito"},
    {"C2","Kemco"},
    {"C3","SquareSoft"},
    {"C4","Tokuma Shoten"},
    {"C5","Data East"},
    {"C6","Tonkin House"},
    {"C8","Koei"},
    {"C9","UPL Comp. Ltd"}, //?
    {"CA","Ultra games/konami"},
    {"CB","Vap Inc."},
    {"CC","USE Co., Ltd"}, //?
    {"CD","Meldac"},
    {"CE","FCI/Pony Canyon"},
    {"CF","Angel Co."},
    {"D0","Taito"}, //?
    {"D1","Sofel"},
    {"D2","Quest"},
    {"D3","Sigma Enterprises"},
    {"D4","Lenar/Ask kodansha"}, //?
    {"D6","Naxat soft"},
    {"D7","Copya system"}, //?
    {"D9","Banpresto"},
    {"DA","Tomy"},
    {"DB","Hiro/LJN"}, //?
    {"DD","NCS/Masiya"}, //?
    {"DE","Human"},
    {"DF","Altron"},
    {"DK","Kodansha"}, //?
    {"E0","KK DCE/Yaleco"}, //?
    {"E1","Towachiki"},
    {"E2","Yutaka"},
    {"E3","Varie"}, //?
    {"E4","T&E SOFT"}, //?
    {"E5","Epoch"},
    {"E7","Athena"},
    {"E8","Asmik"},
    {"E9","Natsume"},
    {"EA","King Records/A Wave"}, //?
    {"EB","Altus"},
    {"EC","Epic/Sony Records"},
    {"EE","IGS Corp"},
    {"EJ","Virgin"},
    {"F0","A Wave/Accolade"}, //?
    {"F3","Extreme Entertainment/Malibu int."}, //?
    {"FB","Psycnosis"}, //?
    {"FF","LJN"} //?
};

const char * GB_GetLicenseeName(char byte1, char byte2)
{
    int i;
    for(i = 0; i < (sizeof(gb_licensee) / sizeof(_licensee_t)); i++)
    {
        if(gb_licensee[i].code[0] == byte1 && gb_licensee[i].code[1] == byte2)
            return gb_licensee[i].name;
    }

    return "Unknown";
}

