python
import sys, os
# Add Qt pretty-printers path
qtdebug_path = os.path.join(os.environ.get('PROJECT_ROOT', os.getcwd()), 'tool', 'qtdebug')
if os.path.isdir(qtdebug_path):
    sys.path.insert(0, qtdebug_path)
    sys.path.insert(0, os.path.join(qtdebug_path, 'printers'))
    from qt import register_qt_printers
    register_qt_printers(None)
    print("[Qt Pretty-Printers] Loaded from: " + qtdebug_path)
else:
    print("[Qt Pretty-Printers] WARNING: Path not found: " + qtdebug_path)
end
