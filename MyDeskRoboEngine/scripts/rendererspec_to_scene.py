#!/usr/bin/env python3
import argparse
import json
import re
from pathlib import Path

LAYER_OP_MAP = {
    'draw_glow': 'LayerOp::DrawGlow',
    'draw_mid': 'LayerOp::DrawMid',
    'draw_core': 'LayerOp::DrawCore',
    'draw_shape': 'LayerOp::DrawShape',
    'draw_cut': 'LayerOp::DrawCut',
    'draw_brow': 'LayerOp::DrawBrow',
}

PRIMITIVE_MAP = {
    'ellipse': 'Primitive::Ellipse',
    'rounded_rect': 'Primitive::RoundedRect',
}

SIDE_MAP = {
    'both': 'Side::Both',
    'left': 'Side::Left',
    'right': 'Side::Right',
}

COLOR_MODE_MAP = {
    'eye': 'ColorMode::RuntimeEye',
    'bg': 'ColorMode::SceneBackground',
    'custom': 'ColorMode::Custom',
}

DEFAULT_BG = '#06080d'
DEFAULT_EYE = '#0fdaff'


def pascal(name: str) -> str:
    parts = re.split(r'[^A-Za-z0-9]+', name)
    return ''.join(p[:1].upper() + p[1:] for p in parts if p)


def cpp_bool(value: bool) -> str:
    return 'true' if value else 'false'


def parse_hex_color(value: str) -> tuple[int, int, int]:
    raw = value.strip().lstrip('#')
    if len(raw) != 6 or not re.fullmatch(r'[0-9a-fA-F]{6}', raw):
        raise ValueError(f'invalid hex color: {value}')
    return int(raw[0:2], 16), int(raw[2:4], 16), int(raw[4:6], 16)


def color_expr(value: str) -> str:
    r, g, b = parse_hex_color(value)
    return f'lv_color_make(0x{r:02X}, 0x{g:02X}, 0x{b:02X})'


def opacity_expr(value: int) -> str:
    if value >= 255:
        return 'LV_OPA_COVER'
    return f'(lv_opa_t){value}'


def build_op(op: dict, fallback_bg: str) -> str:
    layer_op = LAYER_OP_MAP[op['op']]
    primitive = PRIMITIVE_MAP[op['primitive']]
    side = SIDE_MAP[op.get('side', 'both')]
    color = op.get('color', {})
    color_mode = COLOR_MODE_MAP[color.get('mode', 'eye')]
    color_value = color.get('value') or (fallback_bg if color.get('mode') == 'bg' else DEFAULT_EYE)
    runtime_linked = cpp_bool(bool(color.get('runtimeLinked', color.get('mode') == 'eye')))
    enabled = cpp_bool(bool(op.get('enabled', True)))
    mirror_x = cpp_bool(bool(op.get('mirrorXForRight', True)))
    mirror_angle = cpp_bool(bool(op.get('mirrorAngleForRight', True)))
    offset = op.get('offset', {})
    size = op.get('size', {})
    radius = int(op.get('radius', 0))
    angle = int(op.get('angleDeg', 0))
    opacity = int(op.get('opacity', 255))
    name = op.get('name', 'Layer')
    return (
        f'    {{{enabled}, "{name}", {layer_op}, {primitive}, {side}, {mirror_x}, {mirror_angle}, '
        f'{{{int(offset.get("x", 0))}, {int(offset.get("y", 0))}}}, '
        f'{{{int(size.get("w", 0))}, {int(size.get("h", 0))}}}, {radius}, {angle}, '
        f'{{{color_mode}, {color_expr(color_value)}, {runtime_linked}}}, {opacity_expr(opacity)}}},'
    )


def generate_header(spec: dict, animation_symbol: str, scene_symbol: str | None, ops_symbol: str | None) -> str:
    if spec.get('kind') != 'deskrobo.eye_scene':
        raise ValueError('unsupported spec kind')
    scene = spec['scene']
    geometry = scene['geometry']
    palette = scene.get('palette', {})
    scene_name = scene['name']
    base_name = pascal(scene_name)
    ops_name = ops_symbol or f'k{base_name}Ops'
    scene_var = scene_symbol or f'k{base_name}Scene'
    bg = palette.get('bgColor', DEFAULT_BG)
    ops = scene.get('operations', [])
    if not ops:
        raise ValueError('spec does not contain any operations')

    lines = [build_op(op, bg) for op in ops]
    return f'''#pragma once

#include "SceneCommon.h"

namespace nse {{

static const RenderOp {ops_name}[] = {{
{chr(10).join(lines)}
}};

static const EyeSceneSpec {scene_var} = {{
    "{scene_name}",
    {{{int(geometry['virtualWidth'])}, {int(geometry['virtualHeight'])}, {int(geometry['eyeGap'])}, {int(geometry['baseY'])}}},
    {color_expr(bg)},
    {animation_symbol},
    {ops_name},
    sizeof({ops_name}) / sizeof({ops_name}[0]),
}};

}} // namespace nse
'''


def main() -> int:
    parser = argparse.ArgumentParser(description='Convert MyDeskRobo Renderer Spec JSON to nextgen Scene .h')
    parser.add_argument('spec', help='Path to renderer spec JSON')
    parser.add_argument('-o', '--output', help='Output .h path')
    parser.add_argument('--animation', default='kAnimIdle', help='Animation profile symbol, e.g. kAnimIdle')
    parser.add_argument('--scene-symbol', help='Override scene variable symbol, e.g. kEveHappyScene')
    parser.add_argument('--ops-symbol', help='Override ops array symbol, e.g. kEveHappyOps')
    args = parser.parse_args()

    spec_path = Path(args.spec)
    data = json.loads(spec_path.read_text(encoding='utf-8-sig'))
    header = generate_header(data, args.animation, args.scene_symbol, args.ops_symbol)

    if args.output:
        out_path = Path(args.output)
    else:
        out_path = spec_path.with_suffix('.h')

    out_path.write_text(header, encoding='utf-8')
    print(f'Wrote {out_path}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

