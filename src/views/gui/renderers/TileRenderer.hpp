#pragma once

#if NIMONSPOLY_ENABLE_RAYLIB

#include "ui/RaylibCompat.hpp"
#include "utils/Types.hpp"

namespace gui::tile {
    enum class BoardSide {
        BOTTOM,
        LEFT,
        TOP,
        RIGHT,
    };

    int boardEdgeTileCount(int totalTiles);
    BoardSide boardSideForIndex(int tileIndex, int totalTiles);
    Rectangle boardTileBounds(int tileIndex, Rectangle boardBounds, int totalTiles);

    void drawTile(const TileData& tile, Rectangle bounds, BoardSide side);
    void drawPropertyTile(const TileData& tile, Rectangle bounds, BoardSide side);
    void drawSpecialTile(const TileData& tile, Rectangle bounds, BoardSide side);
}

#endif
