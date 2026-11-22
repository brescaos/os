// What the dialogs build their forms with.
#pragma once

#include <QString>

class QAbstractButton;
class QLabel;
class QWidget;

QLabel *createSectionTitle(const QString &text);
// Wrapped, in the placeholder text's color.
QLabel *createExplanation(const QString &text);
void setMonospace(QWidget *widget);
// An error in the color scheme's negative text, or a hint.
void setNegative(QLabel *label, bool isNegative);
// A checkbox or a radio button with what it does under its text, aligned
// with it.
QWidget *createOption(QAbstractButton *button, const QString &explanation);
// path with ~ for the home, as the user would type it.
QString homeRelativePath(const QString &path);
