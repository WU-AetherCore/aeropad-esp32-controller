"""Fail on broken local Markdown links/images in the public documentation set."""
from pathlib import Path
import re,urllib.parse
root=Path(__file__).resolve().parents[1];fail=[];checked=0
files=list(root.glob('*.md'))+list((root/'docs').rglob('*.md'))+[p for p in (root/'examples').rglob('README.md') if '.pio' not in p.parts]
for p in files:
 if p.name=='VALIDATION.md' and p.parent==root:continue # local operational log, not exported
 s=re.sub(r'```[\s\S]*?```','',p.read_text(encoding='utf-8-sig'))
 for link in re.findall(r'!?\[[^\]]*\]\(([^)]+)\)',s):
  target=link.strip().split(' "')[0].strip('<>');parts=urllib.parse.urlsplit(target)
  if parts.scheme or not parts.path:continue
  dest=(p.parent/urllib.parse.unquote(parts.path)).resolve();checked+=1
  if not dest.exists():fail.append(f'{p.relative_to(root)} -> {target}')
if fail:raise SystemExit('Broken documentation links:\n'+'\n'.join(fail))
print(f'PASS: {len(files)} Markdown files, {checked} local document/image links.')
