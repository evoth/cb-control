#include "canonData.h"

CameraPropMap<uint32_t> CanonApertureValues({
    {0x0d, {12, 10}},  {0x10, {14, 10}},  {0x13, {16, 10}},  {0x15, {18, 10}},
    {0x18, {20, 10}},  {0x1b, {22, 10}},  {0x1d, {25, 10}},  {0x20, {28, 10}},
    {0x23, {32, 10}},  {0x25, {35, 10}},  {0x28, {40, 10}},  {0x2b, {45, 10}},
    {0x2d, {50, 10}},  {0x30, {56, 10}},  {0x33, {63, 10}},  {0x35, {71, 10}},
    {0x38, {80, 10}},  {0x3b, {90, 10}},  {0x3d, {100, 10}}, {0x40, {110, 10}},
    {0x43, {130, 10}}, {0x45, {140, 10}}, {0x48, {160, 10}}, {0x4b, {180, 10}},
    {0x4d, {200, 10}}, {0x50, {220, 10}}, {0x53, {250, 10}}, {0x55, {290, 10}},
    {0x58, {320, 10}},
});

CameraPropMap<uint32_t> CanonShutterSpeedValues({
    {0x04, CPV_BULB},  {0x10, {30, 1}},   {0x13, {25, 1}},   {0x15, {20, 1}},
    {0x18, {15, 1}},   {0x1b, {13, 1}},   {0x1d, {10, 1}},   {0x20, {8, 1}},
    {0x23, {6, 1}},    {0x25, {5, 1}},    {0x28, {4, 1}},    {0x2b, {32, 10}},
    {0x2d, {25, 10}},  {0x30, {2, 1}},    {0x33, {16, 10}},  {0x35, {13, 10}},
    {0x38, {1, 1}},    {0x3b, {8, 10}},   {0x3d, {6, 10}},   {0x40, {5, 10}},
    {0x43, {4, 10}},   {0x45, {3, 10}},   {0x48, {1, 4}},    {0x4b, {1, 5}},
    {0x4d, {1, 6}},    {0x50, {1, 8}},    {0x53, {1, 10}},   {0x55, {1, 13}},
    {0x58, {1, 15}},   {0x5b, {1, 20}},   {0x5d, {1, 25}},   {0x60, {1, 30}},
    {0x63, {1, 40}},   {0x65, {1, 50}},   {0x68, {1, 60}},   {0x6b, {1, 80}},
    {0x6d, {1, 100}},  {0x70, {1, 125}},  {0x73, {1, 160}},  {0x75, {1, 200}},
    {0x78, {1, 250}},  {0x7b, {1, 320}},  {0x7d, {1, 400}},  {0x80, {1, 500}},
    {0x83, {1, 640}},  {0x85, {1, 800}},  {0x88, {1, 1000}}, {0x8b, {1, 1250}},
    {0x8d, {1, 1600}}, {0x90, {1, 2000}}, {0x93, {1, 2500}}, {0x95, {1, 3200}},
    {0x98, {1, 4000}}, {0x9a, {1, 5000}}, {0x9d, {1, 6400}}, {0xA0, {1, 8000}},
});

// TODO: Try to find more codes (I can get the value for 25600 with M200)
CameraPropMap<uint32_t> CanonISOValues({
    {0x40, {50, 1}},    {0x48, {100, 1}},   {0x4b, {125, 1}},
    {0x4d, {160, 1}},   {0x50, {200, 1}},   {0x53, {250, 1}},
    {0x55, {320, 1}},   {0x58, {400, 1}},   {0x5b, {500, 1}},
    {0x5d, {640, 1}},   {0x60, {800, 1}},   {0x63, {1000, 1}},
    {0x65, {1250, 1}},  {0x68, {1600, 1}},  {0x6b, {2000, 1}},
    {0x6d, {2500, 1}},  {0x70, {3200, 1}},  {0x73, {4000, 1}},
    {0x75, {5000, 1}},  {0x78, {6400, 1}},  {0x7b, {8000, 1}},
    {0x7d, {10000, 1}}, {0x80, {12800, 1}},
});