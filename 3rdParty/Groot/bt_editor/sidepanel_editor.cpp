#include "sidepanel_editor.h"
#include "ui_sidepanel_editor.h"
#include "custom_node_dialog.h"
#include "utils.h"

#include <QHeaderView>
#include <QPushButton>
#include <QSettings>
#include <QFileInfo>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSettings>

SidepanelEditor::SidepanelEditor(QtNodes::DataModelRegistry *registry,
                                 NodeModels &tree_nodes_model,
                                 QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelEditor),
    _tree_nodes_model(tree_nodes_model),
    _model_registry(registry)
{
    ui->setupUi(this);   
    ui->paramsFrame->setHidden(true);
    ui->paletteTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect( ui->paletteTreeWidget, &QWidget::customContextMenuRequested,
             this, &SidepanelEditor::onContextMenu);

    connect(ui->paletteTreeWidget, &QTreeWidget::itemDoubleClicked,
            this, &SidepanelEditor::onDoubleClick);

    auto table_header = ui->portsTableWidget->horizontalHeader();

    table_header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_header->setSectionResizeMode(1, QHeaderView::Interactive);
    table_header->setSectionResizeMode(2, QHeaderView::Stretch);

    ui->buttonLock->setChecked(false);

    QSettings settings;
    table_header->restoreState( settings.value("SidepanelEditor/header").toByteArray() );
}

SidepanelEditor::~SidepanelEditor()
{
    QSettings settings;
    settings.setValue("SidepanelEditor/header",
                      ui->portsTableWidget->horizontalHeader()->saveState() );

    delete ui;
}

void SidepanelEditor::updateTreeView()
{
    ui->paletteTreeWidget->clear();
    _tree_view_category_items.clear();

    for (const QString& category : {"Action", "Condition",
                                    "Control", "Decorator", "SubTree" } )
    {
      // Map English category names to Chinese for display
      static const QMap<QString, QString> categoryDisplayNames = {
          {"Action", QString::fromUtf8("\u52A8\u4F5C")},
          {"Condition", QString::fromUtf8("\u6761\u4EF6")},
          {"Control", QString::fromUtf8("\u63A7\u5236")},
          {"Decorator", QString::fromUtf8("\u4FEE\u9970\u5668")},
          {"SubTree", QString::fromUtf8("\u5B50\u6811")}
      };
      QString displayName = categoryDisplayNames.value(category, category);
      auto item = new QTreeWidgetItem(ui->paletteTreeWidget, {displayName});
      QFont font = item->font(0);
      font.setBold(true);
      font.setPointSize(11);
      item->setFont(0, font);
      item->setFlags( item->flags() ^ Qt::ItemIsDragEnabled );
      item->setFlags( item->flags() ^ Qt::ItemIsSelectable );
      _tree_view_category_items[ category ] = item;
    }

    for (const auto &it : _tree_nodes_model)
    {
      const auto& ID = it.first;
      const NodeModel& model = it.second;

      if( model.registration_ID == "Root")
      {
          continue;
      }
      QString category = QString::fromStdString(toStr(model.type));
      auto parent = _tree_view_category_items[category];
      auto item = new QTreeWidgetItem(parent, {ID});
      const bool is_builtin = BuiltinNodeModels().count( ID ) > 0;
      const bool is_editable = (!ui->buttonLock->isChecked() && !is_builtin);

      QFont font = item->font(0);
      font.setItalic( is_builtin );
      font.setPointSize(11);
      item->setFont(0, font);
      item->setData(0, Qt::UserRole, ID);

      if (is_editable)
      {
        item->setForeground(0, QBrush(QColor(70, 110, 154)));
      }
    }

    ui->paletteTreeWidget->expandAll();
}

void SidepanelEditor::clear()
{

}

void SidepanelEditor::on_paletteTreeWidget_itemSelectionChanged()
{
  auto selected_items = ui->paletteTreeWidget->selectedItems();
  if(selected_items.size() == 0)
  {
    ui->paramsFrame->setHidden(true);
  }
  else {
    auto selected_item = selected_items.front();
    QString item_name = selected_item->text(0);
    ui->paramsFrame->setHidden(false);
    ui->label->setText( item_name + QString::fromUtf8(" \u53C2\u6570"));

    const auto& model = _tree_nodes_model.at(item_name);

    ui->portsTableWidget->setRowCount( model.ports.size() );

    int row = 0;
    for (const auto& port_it: model.ports)
    {
        ui->portsTableWidget->setItem(row,0, new QTableWidgetItem( QString::fromStdString(toStr(port_it.second.direction))));
        ui->portsTableWidget->setItem(row,1, new QTableWidgetItem( port_it.first ));
        ui->portsTableWidget->setItem(row,2, new QTableWidgetItem( port_it.second.description) );
        row++;
    }
    ui->portsTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  }

}

void SidepanelEditor::on_lineEditFilter_textChanged(const QString &text)
{
  for (auto& it : _tree_view_category_items)
  {
    for (int i = 0; i < it.second->childCount(); ++i)
    {
      auto child = it.second->child(i);
      auto modelName = child->data(0, Qt::UserRole).toString();
      bool show = modelName.contains(text, Qt::CaseInsensitive);
      child->setHidden( !show);
    }
  }
}


void SidepanelEditor::on_buttonAddNode_clicked()
{
    CustomNodeDialog dialog(_tree_nodes_model, QString(), this);
    if( dialog.exec() == QDialog::Accepted)
    {
        auto new_model = dialog.getTreeNodeModel();
        if( new_model.type == NodeType::SUBTREE )
        {
            emit addSubtree( new_model.registration_ID );
        }
        emit addNewModel( new_model );
    }
    updateTreeView();
}

void SidepanelEditor::onRemoveModel(QString selected_name)
{
    NodeType node_type = _tree_nodes_model.at(selected_name).type;

    _tree_nodes_model.erase( selected_name );
    _model_registry->unregisterModel(selected_name);
    updateTreeView();
    if( node_type == NodeType::SUBTREE)
    {
        emit destroySubtree(selected_name);
    }
}



void SidepanelEditor::onContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* selected_item = ui->paletteTreeWidget->itemAt(pos);
    if( selected_item == nullptr)
    {
        return;
    }
    QString selected_name = selected_item->text(0);

    if( ui->buttonLock->isChecked() ||
        BuiltinNodeModels().count( selected_name ) != 0 )
    {
        return;
    }

    // Loop through the category items and prevent the right click
    // menu from showing for any of the items
    for (const auto& it : _tree_view_category_items)
    {
        const auto category_item = it.second;
        if( category_item == selected_item ) {
            return;
        }
    }

    QMenu menu(this);

    QAction* edit   = menu.addAction(QString::fromUtf8("\u7F16\u8F91"));
    connect( edit, &QAction::triggered, this, [this, selected_name]()
            {
                CustomNodeDialog dialog(_tree_nodes_model, selected_name, this);
                if( dialog.exec() == QDialog::Accepted)
                {
                    onReplaceModel( selected_name, dialog.getTreeNodeModel() );
                }
            } );

    QAction* remove = menu.addAction(QString::fromUtf8("\u5220\u9664"));

    connect( remove, &QAction::triggered, this,[this, selected_name]()
    {
        emit modelRemoveRequested(selected_name);
    } );

    QPoint globalPos = ui->paletteTreeWidget->mapToGlobal(pos);
    menu.exec(globalPos);

    QApplication::processEvents();
}

void SidepanelEditor::onReplaceModel(const QString& old_name,
                                     const NodeModel &new_model)
{
    _tree_nodes_model.erase( old_name );
    _model_registry->unregisterModel( old_name );
    emit addNewModel( new_model );

    if( new_model.type == NodeType::SUBTREE )
    {
       emit renameSubtree(old_name, new_model.registration_ID);
    }

    emit nodeModelEdited(old_name, new_model.registration_ID);
}


void SidepanelEditor::on_buttonUpload_clicked()
{
    QDomDocument doc;

    QDomElement root = doc.createElement( "root" );
    doc.appendChild( root );

    QDomElement root_models = doc.createElement("TreeNodesModel");

    for(const auto& tree_it: _tree_nodes_model)
    {
        const auto& ID    = tree_it.first;
        const auto& model = tree_it.second;

        if( BuiltinNodeModels().count(ID) != 0 )
        {
            continue;
        }

        QDomElement node = doc.createElement( QString::fromStdString(toStr(model.type)) );

        if( !node.isNull() )
        {
            node.setAttribute("ID", ID.toStdString().c_str());
            for(const auto& port_it: model.ports)
            {
                node.appendChild(writePortModel(port_it.first, port_it.second, doc));
            }
        }
        root_models.appendChild(node);
    }
    root.appendChild(root_models);

    //-------------------------------------
    QSettings settings;
    QString directory_path  = settings.value("SidepanelEditor.lastSaveDirectory",
                                             QDir::currentPath() ).toString();

    auto fileName = QFileDialog::getSaveFileName(this,QString::fromUtf8("\u4FDD\u5B58\u884C\u4E3A\u6811\u5230\u6587\u4EF6"),
                                                 directory_path,QString::fromUtf8("\u884C\u4E3A\u6811\u6587\u4EF6 (*.xml)"));
    if (fileName.isEmpty()){
        return;
    }
    if (!fileName.endsWith(".xml"))
    {
        fileName += ".xml";
    }


    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << doc.toString(4) << Qt::endl;
    }

    directory_path = QFileInfo(fileName).absolutePath();
    settings.setValue("SidepanelEditor.lastSaveDirectory", directory_path);

}

void SidepanelEditor::on_buttonDownload_clicked()
{
    QSettings settings;
    QString directory_path  = settings.value("SidepanelEditor.lastLoadDirectory",
                                             QDir::homePath() ).toString();

    QString fileName = QFileDialog::getOpenFileName(this, tr("从文件加载节点模型"),
                                                    directory_path,
                                                    tr("行为树 (*.xml *.skills.json)" ));
    QFileInfo fileInfo(fileName);

    if (!fileInfo.exists(fileName)){
        return;
    }

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)){
        return;
    }

    directory_path = QFileInfo(fileName).absolutePath();
    settings.setValue("SidepanelEditor.lastLoadDirectory", directory_path);
    settings.sync();

    //--------------------------------
    NodeModels imported_models;
    if( fileInfo.suffix() == "xml" )
    {
        QFile file(fileName);
        imported_models = importFromXML( &file );
    }
    else if( fileInfo.completeSuffix() == "skills.json" )
    {
        imported_models = importFromSkills( fileName );
    }

    if( imported_models.empty() )
    {
        return;
    }

    auto models_to_remove = GetModelsToRemove(this, _tree_nodes_model, imported_models );

    for(QString model_name: models_to_remove)
    {
        emit modelRemoveRequested(model_name);
    }

    for(auto& it: imported_models)
    {
        emit addNewModel( it.second );
    }
}

NodeModels SidepanelEditor::importFromXML(QFile* file)
{
    QDomDocument doc;


    if (!file->open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,QString::fromUtf8("\u52A0\u8F7D\u8282\u70B9\u6A21\u578B\u51FA\u9519"),
                             QString::fromUtf8("XML\u6587\u4EF6\u52A0\u8F7D\u5931\u8D25"));
        return {};
    }

    QString errorMsg;
    int errorLine;
    if( ! doc.setContent(file, &errorMsg, &errorLine ) )
    {
        auto error = tr("解析XML错误 (第%1行): %2").arg(errorLine).arg(errorMsg);
        QMessageBox::warning(this,QString::fromUtf8("\u52A0\u8F7D\u8282\u70B9\u6A21\u578B\u51FA\u9519"), error);
        file->close();
        return {};
    }
    file->close();

    NodeModels custom_models;

    QDomElement xml_root = doc.documentElement();
    if ( xml_root.isNull() || xml_root.tagName() != "root")
    {
        QMessageBox::warning(this,QString::fromUtf8("\u52A0\u8F7D\u8282\u70B9\u6A21\u578B\u51FA\u9519"),
                             QString::fromUtf8("XML\u5FC5\u987B\u6709\u4E00\u4E2A\u540D\u4E3A <root> \u7684\u6839\u8282\u70B9"));
        return custom_models;
    }

    auto manifest_root = xml_root.firstChildElement("TreeNodesModel");

    if ( manifest_root.isNull() )
    {
        QMessageBox::warning(this,QString::fromUtf8("\u52A0\u8F7D\u8282\u70B9\u6A21\u578B\u51FA\u9519"),
                             QString::fromUtf8("\u5728 <root> \u4E0B\u627E\u4E0D\u5230 <TreeNodesModel>"));
        return custom_models;
    }

    for( QDomElement model_element = manifest_root.firstChildElement();
         !model_element.isNull();
         model_element = model_element.nextSiblingElement() )
    {
        auto model = buildTreeNodeModelFromXML(model_element);
        custom_models.insert( { model.registration_ID, model } );
    }

    return custom_models;
}

NodeModels SidepanelEditor::importFromSkills(const QString &fileName)
{
    NodeModels custom_models;

    QFile loadFile(fileName);

    if (!loadFile.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this,QString::fromUtf8("\u52A0\u8F7DSkills\u51FA\u9519"),
                             tr("文件 %1 加载出错").arg(fileName) );
        return custom_models;
    }

    // TODO VER_3
//     QJsonDocument loadDoc =  QJsonDocument::fromJson( loadFile.readAll() ) ;

//     QJsonArray root_array = loadDoc.array();

//     for (QJsonValueRef skill_node : root_array)
//     {

//         auto skill = skill_node.toObject()["skill"].toObject();
//         auto name = skill["name"].toString();
//         qDebug() << name;

//         auto attributes = skill["in-attribute"].toObject();
//         auto params_keys = attributes.keys();

//         PortModels ports_models;

//         for (const auto& key: params_keys)
//         {
//             ports_models.insert(  {key, attributes[key].toString()} );
//         }
//         NodeModel model = { NodeType::ACTION, name, ports_mapping };
//         custom_models.insert( {name, model} );
//     }

    return custom_models;
}


void SidepanelEditor::on_buttonLock_toggled(bool locked)
{
    static QIcon icon_locked( QPixmap(":/icons/svg/lock.svg" ) );
    static QIcon icon_unlocked( QPixmap(":/icons/svg/lock_open.svg") );

    ui->buttonLock->setIcon( locked ? icon_locked : icon_unlocked);
    updateTreeView();
}

void SidepanelEditor::onDoubleClick(QTreeWidgetItem *item, int column)
{
    QString selected_name = item->text(0);

    if( ui->buttonLock->isChecked() ||
        BuiltinNodeModels().count( selected_name ) != 0 )
    {
        return;
    }

    if (item->parent() && item->parent()->text(0) == "SubTree")
    {
        emit setTabScope(selected_name);
    }
}
