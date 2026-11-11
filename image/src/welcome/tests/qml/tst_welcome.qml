// The welcome window: its pages load, the steps go between the welcome page
// and the last one, and Skip / Back / Next walk through them.
import QtQuick
import QtTest
import com.brescaos.welcome

TestCase {
    id: testCase
    name: "Welcome"
    when: windowShown

    readonly property url sampleStep: Qt.resolvedUrl("SampleStep.qml")

    Component {
        id: mainComponent
        Main {}
    }

    function createWindow(steps) {
        const window = createTemporaryObject(mainComponent, null);
        verify(window, "Main.qml didn't load");
        // Not as an initial property: createObject leaves a list<url> one empty.
        window.steps = steps;
        return window;
    }

    function loadedPage(window, index) {
        const loader = window.pageView.itemAt(index);
        tryCompare(loader, "status", Loader.Ready, 5000, window.pages[index] + " didn't load");
        return loader.item;
    }

    function test_withoutSteps_welcomeAndDone() {
        const window = createWindow([]);
        compare(window.pageView.count, 2);
        compare(loadedPage(window, 0).title, "Welcome to BrescaOS 1.2.3");
        compare(loadedPage(window, 1).title, "You're all set");
    }

    function test_stepsGoBetweenWelcomeAndDone() {
        const window = createWindow([testCase.sampleStep, testCase.sampleStep]);
        compare(window.pageView.count, 4);
        compare(loadedPage(window, 1).title, "Sample step");
        compare(loadedPage(window, 2).title, "Sample step");
        compare(loadedPage(window, 3).title, "You're all set");
    }

    // At the default window size no page needs a scroll bar. 10% headroom:
    // offscreen fonts and the footer are a bit smaller than in Plasma.
    function test_noPageScrollsAtDefaultSize() {
        const window = createWindow([testCase.sampleStep]);
        for (let i = 0; i < window.pages.length; ++i) {
            const page = loadedPage(window, i);
            // Once laid out: text wraps after the page gets its width.
            tryVerify(() => page.height > 0 && page.neededHeight <= page.height * 0.9, 2000,
                      window.pages[i] + " scrolls: " + page.neededHeight + " > 90% of " + page.height);
        }
    }

    function test_navigation() {
        const window = createWindow([testCase.sampleStep]);
        const back = window.backButton;
        const next = window.nextButton;
        waitForRendering(window.contentItem);

        verify(window.atStart);
        compare(back.text, "Skip");
        compare(next.text, "Next");

        mouseClick(next);
        compare(window.pageView.currentIndex, 1);
        compare(back.text, "Back");

        mouseClick(back);
        compare(window.pageView.currentIndex, 0);

        mouseClick(next);
        mouseClick(next);
        compare(window.pageView.currentIndex, 2);
        verify(window.atEnd);
        compare(next.text, "Start using BrescaOS");
    }

    // Required by Fedora's trademark guidelines (README.md, "Fedora notice").
    function test_welcomeShowsFedoraNotice() {
        const window = createWindow([]);
        const page = loadedPage(window, 0);
        const notice = findChild(page, "fedoraNotice");
        verify(notice, "no Fedora notice on the welcome page");
        verify(notice.visible);
        verify(notice.text.indexOf("not provided or supported by the Fedora Project") >= 0, notice.text);
        verify(notice.text.indexOf('href="https://fedoraproject.org/"') >= 0, notice.text);
        // At the bottom of the page, below the rest of its content.
        tryVerify(() => page.height > 0 && Math.abs(notice.mapToItem(page, 0, notice.height).y - (page.height - page.margin)) < 1,
                  2000, "the Fedora notice isn't at the bottom of the page");
    }
}
