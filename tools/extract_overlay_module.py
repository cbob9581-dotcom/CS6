from pathlib import Path
p=Path('D:/CS6/Plugins/CS6BombRuntime/Source/CS6BombRuntime/Private/CS6TacticalMatch.cpp')
t=p.read_text(encoding='utf-8');i=t.index('int32 UCS6RoundOverlay::NativePaint(')
p.write_text(t[:i],encoding='utf-8')
