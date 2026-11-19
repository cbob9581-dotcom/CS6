from pathlib import Path
path=Path('D:/CS6/Plugins/CS6BombRuntime/Source/CS6BombRuntime/Private/CS6TacticalMatch.cpp')
text=path.read_text(encoding='utf-8')
start=text.index('void ACS6TacticalMatch::UpdateObjectives() {')
end=text.index('void ACS6TacticalMatch::RoundEnded_Implementation',start)
path.write_text(text[:start]+text[end:],encoding='utf-8')
