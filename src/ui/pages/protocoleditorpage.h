// 协议编辑页面（ProtocolEditorPage）
// ------------------------------------
// 可视化编辑 .bin 协议描述：增删改字段、调整顺序、另存为 .bin。
//
// 设计：
// - 字段表用 ProtocolFieldTableModel（可编辑）。
// - 右侧详情面板镜像选中行，"应用"按钮统一回写，避免逐键修改造成模型抖动。
// - 偏移列只读（由用户在详情面板设置，或后续实现自动布局）。

#pragma once

#include "placeholderpagebase.h"
#include "models/binprotocolloader.h"

#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class ProtocolEditorPage;
}
QT_END_NAMESPACE

class ProtocolFieldTableModel;

class ProtocolEditorPage : public PlaceholderPageBase
{
    Q_OBJECT

public:
    explicit ProtocolEditorPage(QWidget *parent = nullptr);
    ~ProtocolEditorPage();

private:
    void newProtocol();
    void openFile();
    void saveFile();
    void saveAsFile();
    void loadSample();
    void applyDetailToCurrentRow();
    void onSelectionChanged();
    void refreshFrameSize();

    void setFilePath(const QString &path);

private:
    Ui::ProtocolEditorPage *ui;
    ProtocolFieldTableModel *m_model = nullptr;
    BinProtocolLoader m_loader;
    QString m_filePath;
};
