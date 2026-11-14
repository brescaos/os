// The clock's date and time formatting, shared by the clock (main.qml) and
// its settings page's preview (configGeneral.qml).
.pragma library

// Locale.ShortFormat. A library script has no QML context, so it can't
// name the Locale enum: using it there throws, and the clock's labels end
// up empty.
const SHORT_FORMAT = 1;

// The time format for a timeFormat setting: 0 as the locale has it, 1
// 12-hour, 2 24-hour.
function timeFormat(locale, setting) {
    switch (setting) {
    case 1:
        return "h:mm AP";
    case 2:
        return "HH:mm";
    default:
        return locale.timeFormat(SHORT_FORMAT);
    }
}

// dateTime in format, Qt's date format syntax; an empty format is the
// region's short date. Some languages end their abbreviated weekdays and
// months with a dot ("Mi." in German); ddd and MMM drop it, the rest of the
// format is kept as written.
function dateText(locale, dateTime, format) {
    if (format === "") {
        format = locale.dateFormat(SHORT_FORMAT);
    }
    const quote = name => "'" + name.replace(/\.$/, "").replace(/'/g, "''") + "'";
    const shortNames = format.replace(/(d+|M+)/g, token => {
        if (token === "ddd") {
            return quote(locale.dayName(dateTime.getDay(), SHORT_FORMAT));
        }
        if (token === "MMM") {
            return quote(locale.monthName(dateTime.getMonth(), SHORT_FORMAT));
        }
        return token;
    });
    return locale.toString(dateTime, shortNames);
}
