// A step for the tests only, as a real one is written (README, "Adding a step").
import QtQuick.Controls as QQC2
import com.brescaos.welcome

StepPage {
    title: "Sample step"

    QQC2.CheckBox {
        text: "A setting"
    }
}
