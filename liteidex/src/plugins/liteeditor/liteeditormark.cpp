/**************************************************************************
** This file is part of LiteIDE
**
** Copyright (c) 2011-2016 LiteIDE Team. All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** In addition, as a special exception,  that plugins developed for LiteIDE,
** are allowed to remain closed sourced and can be distributed under any license .
** These rights are included in the file LGPL_EXCEPTION.txt in this package.
**
**************************************************************************/
// Module: liteeditormark.cpp
// Creator: visualfc <visualfc@gmail.com>

#include "liteeditormark.h"
#include "qtc_texteditor/basetextdocumentlayout.h"
#include "liteeditorwidget.h"
#include <QTextDocument>
#include <QTextBlock>
#include <QDebug>
//lite_memory_check_begin
#if defined(WIN32) && defined(_MSC_VER) &&  defined(_DEBUG)
     #define _CRTDBG_MAP_ALLOC
     #include <stdlib.h>
     #include <crtdbg.h>
     #define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
     #define new DEBUG_NEW
#endif
//lite_memory_check_end

LiteTextMark::LiteTextMark(LiteEditorMark *editorMark, int type, int lineNumber, QObject *parent)
    : TextEditor::ITextMark(parent),
      m_editorMark(editorMark),
      m_type(type)
{
    m_lineNumber = lineNumber;
}

LiteTextMark::~LiteTextMark()
{
  //  qDebug() << "~LiteTextMark" << m_type << this->lineNumber();
}

void LiteTextMark::removedFromEditor()
{
    m_editorMark->removedFromEditor(this);
}

void LiteTextMark::updateLineNumber(int lineNumber)
{
    if (m_lineNumber == lineNumber) {
        return;
    }
    int old = m_lineNumber;
    m_lineNumber = lineNumber;
    m_editorMark->updateLineNumber(this, old);
}


LiteEditorMarkTypeManager::LiteEditorMarkTypeManager(QObject *parent) :
    LiteApi::IEditorMarkTypeManager(parent)
{
}

LiteEditorMarkTypeManager::~LiteEditorMarkTypeManager()
{
}

void LiteEditorMarkTypeManager::registerMark(int type, const QIcon &icon)
{
    m_typeIconMap.insert(type,icon);
}

QList<int> LiteEditorMarkTypeManager::markTypeList() const
{
    return m_typeIconMap.keys();
}

QIcon LiteEditorMarkTypeManager::iconForType(int type) const
{
    return m_typeIconMap.value(type);
}

LiteEditorMark::LiteEditorMark(LiteEditorMarkTypeManager *manager, LiteEditor *editor) :
    LiteApi::IEditorMark(editor),
    m_manager(manager),
    m_editor(editor),
    m_document(editor->document())
{
}

static TextEditor::ITextMark *findMarkByType(TextEditor::TextBlockUserData *data, int type)
{
    foreach(TextEditor::ITextMark *mark, data->marks()) {
        if ( ((LiteTextMark*)mark)->type() == type ) {
            return mark;
        }
    }
    return 0;
}

TextEditor::ITextMark *LiteEditorMark::createMarkByType(int type, int line)
{
    QIcon icon = m_manager->iconForType(type);
    if (icon.isNull()) {
        return 0;
    }
    LiteTextMark *mark = new LiteTextMark(this,type,line,this);
    mark->setIcon(icon);
    return mark;
}

void LiteEditorMark::removedFromEditor(TextEditor::ITextMark *mark)
{
    int type = ((LiteTextMark*)mark)->type();
    m_typeLineMarkMap[type].remove(mark->lineNumber());

    emit markListChanged(type);

    delete mark;
}

void LiteEditorMark::updateLineNumber(TextEditor::ITextMark *mark, int oldLineNumber)
{
    int type = ((LiteTextMark*)mark)->type();
    m_typeLineMarkMap[type].remove(oldLineNumber);
    m_typeLineMarkMap[type].insert(mark->lineNumber(),mark);

    emit markListChanged(type);
}

void LiteEditorMark::addMark(int line, int type)
{
    const QTextBlock &block = m_document->findBlockByNumber(line);
    if (!block.isValid()) {
        return;
    }
    TextEditor::TextBlockUserData *data = TextEditor::BaseTextDocumentLayout::userData(block);
    if (!data) {
        return;
    }
    TextEditor::ITextMark *findMark = findMarkByType(data,type);
    if (findMark) {
        return;
    }
    TextEditor::ITextMark *mark = createMarkByType(type, line);
    data->addMark(mark);

    m_typeLineMarkMap[type].insert(line,mark);
    emit markListChanged(type);
}


void LiteEditorMark::removeMark(int line, int type)
{
    const QTextBlock &block = m_document->findBlockByNumber(line);
    if (!block.isValid()) {
        return;
    }
    TextEditor::TextBlockUserData *data = TextEditor::BaseTextDocumentLayout::testUserData(block);
    if (!data) {
        return;
    }
    TextEditor::ITextMark *mark = findMarkByType(data,type);
    if (!mark) {
        return;
    }
    if (data->removeMark(mark)) {
        this->removedFromEditor(mark);
    }
}

QList<int> LiteEditorMark::markList(int type) const
{
 //   qDebug
    return m_typeLineMarkMap[type].keys();
//    QList<int> lineList;
//    QTextBlock block = m_document->firstBlock();
//    while (block.isValid()) {
//        TextEditor::TextBlockUserData *data = TextEditor::BaseTextDocumentLayout::testUserData(block);
//        if (data) {
//            foreach(TextEditor::ITextMark *mark, data->marks() ) {
//                if ( ((LiteTextMark*)mark)->type() == type ) {
//                    lineList.append(block.blockNumber());
//                    break;
//                }
//            }
//        }
//        block = block.next();
//    }
//    return lineList;
}

QList<int> LiteEditorMark::lineTypeList(int line) const
{
    QList<int> typeList;
    const QTextBlock &block = m_document->findBlockByNumber(line);
    if (!block.isValid()) {
        return typeList;
    }
    TextEditor::TextBlockUserData *data = TextEditor::BaseTextDocumentLayout::testUserData(block);
    if (data) {
        foreach(TextEditor::ITextMark *mark, data->marks()) {
            typeList.append(((LiteTextMark*)mark)->type());
        }
    }
    return typeList;
}
