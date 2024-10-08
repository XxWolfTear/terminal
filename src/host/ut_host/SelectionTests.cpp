// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"
#include "WexTestClass.h"
#include "../../inc/consoletaeftemplates.hpp"

#include "CommonState.hpp"

#include "globals.h"

#include "selection.hpp"

#include "../interactivity/inc/ServiceLocator.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;
using Microsoft::Console::Interactivity::ServiceLocator;

class SelectionTests
{
    TEST_CLASS(SelectionTests);

    CommonState* m_state;
    Selection* m_pSelection;

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_state = new CommonState();

        m_state->PrepareGlobalScreenBuffer();

        m_pSelection = &Selection::Instance();
        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        m_pSelection = nullptr;

        m_state->CleanupGlobalScreenBuffer();

        delete m_state;

        return true;
    }

    TEST_METHOD_SETUP(MethodSetup)
    {
        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        return true;
    }

    void VerifyGetSelectionRects_BoxMode()
    {
        const auto selectionRects = m_pSelection->GetSelectionRects();
        const UINT cRectanglesExpected = m_pSelection->_d->srSelectionRect.bottom - m_pSelection->_d->srSelectionRect.top + 1;

        if (VERIFY_ARE_EQUAL(cRectanglesExpected, selectionRects.size()))
        {
            for (auto iRect = 0; iRect < gsl::narrow<int>(selectionRects.size()); iRect++)
            {
                // ensure each rectangle is exactly the width requested (block selection)
                const auto psrRect = &selectionRects[iRect];

                const auto sRectangleLineNumber = (til::CoordType)iRect + m_pSelection->_d->srSelectionRect.top;

                VERIFY_ARE_EQUAL(psrRect->top, sRectangleLineNumber);
                VERIFY_ARE_EQUAL(psrRect->bottom, sRectangleLineNumber);

                VERIFY_ARE_EQUAL(psrRect->left, m_pSelection->_d->srSelectionRect.left);
                VERIFY_ARE_EQUAL(psrRect->right, m_pSelection->_d->srSelectionRect.right);
            }
        }
    }

    TEST_METHOD(TestGetSelectionRects_BoxMode)
    {
        {
            auto selection{ m_pSelection->_d.write() };
            selection->fSelectionVisible = true;

            // set selection region
            selection->srSelectionRect.top = 0;
            selection->srSelectionRect.bottom = 3;
            selection->srSelectionRect.left = 1;
            selection->srSelectionRect.right = 10;

            // #1 top-left to bottom right selection first
            selection->coordSelectionAnchor.x = selection->srSelectionRect.left;
            selection->coordSelectionAnchor.y = selection->srSelectionRect.top;

            // A. false/false for the selection modes should mean box selection
            selection->fLineSelection = false;
            selection->fUseAlternateSelection = false;

            VerifyGetSelectionRects_BoxMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // B. true/true for the selection modes should also mean box selection
            selection->fLineSelection = true;
            selection->fUseAlternateSelection = true;

            VerifyGetSelectionRects_BoxMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // now try the other 3 configurations of box region.
            // #2 top-right to bottom-left selection
            selection->coordSelectionAnchor.x = selection->srSelectionRect.right;
            selection->coordSelectionAnchor.y = selection->srSelectionRect.top;

            VerifyGetSelectionRects_BoxMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // #3 bottom-left to top-right selection
            selection->coordSelectionAnchor.x = selection->srSelectionRect.left;
            selection->coordSelectionAnchor.y = selection->srSelectionRect.bottom;

            VerifyGetSelectionRects_BoxMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // #4 bottom-right to top-left selection
            selection->coordSelectionAnchor.x = selection->srSelectionRect.right;
            selection->coordSelectionAnchor.y = selection->srSelectionRect.bottom;

            VerifyGetSelectionRects_BoxMode();
        }
    }

    void VerifyGetSelectionRects_LineMode()
    {
        const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

        const auto selectionRects = m_pSelection->GetSelectionRects();
        const UINT cRectanglesExpected = m_pSelection->_d->srSelectionRect.bottom - m_pSelection->_d->srSelectionRect.top + 1;

        if (VERIFY_ARE_EQUAL(cRectanglesExpected, selectionRects.size()))
        {
            // RULES:
            // 1. If we're only selection one line, select the entire region between the two rectangles.
            //    Else if we're selecting multiple lines...
            // 2. Extend all lines except the last line to the right edge of the screen
            //    Extend all lines except the first line to the left edge of the screen
            // 3. If our anchor is in the top-right or bottom-left corner of the rectangle...
            //    The inside portion of our rectangle on the first and last lines is invalid.
            //    Remove from selection (but preserve the anchors themselves).

            // RULE #1: If 1 line, entire region selected.
            auto fHaveOneLine = selectionRects.size() == 1;

            if (fHaveOneLine)
            {
                auto srSelectionRect = m_pSelection->_d->srSelectionRect;
                VERIFY_ARE_EQUAL(srSelectionRect.top, srSelectionRect.bottom);

                const auto psrRect = &selectionRects[0];

                VERIFY_ARE_EQUAL(psrRect->top, srSelectionRect.top);
                VERIFY_ARE_EQUAL(psrRect->bottom, srSelectionRect.bottom);

                VERIFY_ARE_EQUAL(psrRect->left, srSelectionRect.left);
                VERIFY_ARE_EQUAL(psrRect->right, srSelectionRect.right);
            }
            else
            {
                // RULE #2 : Check extension to edges
                for (UINT iRect = 0; iRect < selectionRects.size(); iRect++)
                {
                    // ensure each rectangle is exactly the width requested (block selection)
                    const auto psrRect = &selectionRects[iRect];

                    const auto sRectangleLineNumber = (til::CoordType)iRect + m_pSelection->_d->srSelectionRect.top;

                    VERIFY_ARE_EQUAL(psrRect->top, sRectangleLineNumber);
                    VERIFY_ARE_EQUAL(psrRect->bottom, sRectangleLineNumber);

                    auto fIsFirstLine = iRect == 0;
                    auto fIsLastLine = iRect == selectionRects.size() - 1;

                    // for all lines except the last, the line should reach the right edge of the buffer
                    if (!fIsLastLine)
                    {
                        // buffer size = 80, then selection goes 0 to 79. Thus X - 1.
                        VERIFY_ARE_EQUAL(psrRect->right, gci.GetActiveOutputBuffer().GetTextBuffer().GetSize().RightInclusive());
                    }

                    // for all lines except the first, the line should reach the left edge of the buffer
                    if (!fIsFirstLine)
                    {
                        VERIFY_ARE_EQUAL(psrRect->left, 0);
                    }
                }

                // RULE #3: Check first and last line have invalid regions removed, if applicable
                UINT iFirst = 0;
                auto iLast = gsl::narrow<UINT>(selectionRects.size() - 1u);

                const auto psrFirst = &selectionRects[iFirst];
                const auto psrLast = &selectionRects[iLast];

                auto fRemoveRegion = false;

                auto srSelectionRect = m_pSelection->_d->srSelectionRect;
                auto coordAnchor = m_pSelection->_d->coordSelectionAnchor;

                // if the anchor is in the top right or bottom left corner, we must have removed a region. otherwise, it stays as is.
                if (coordAnchor.y == srSelectionRect.top && coordAnchor.x == srSelectionRect.right)
                {
                    fRemoveRegion = true;
                }
                else if (coordAnchor.y == srSelectionRect.bottom && coordAnchor.x == srSelectionRect.left)
                {
                    fRemoveRegion = true;
                }

                // now check the first row's left based on removal
                if (!fRemoveRegion)
                {
                    VERIFY_ARE_EQUAL(psrFirst->left, srSelectionRect.left);
                }
                else
                {
                    VERIFY_ARE_EQUAL(psrFirst->left, srSelectionRect.right);
                }

                // and the last row's right based on removal
                if (!fRemoveRegion)
                {
                    VERIFY_ARE_EQUAL(psrLast->right, srSelectionRect.right);
                }
                else
                {
                    VERIFY_ARE_EQUAL(psrLast->right, srSelectionRect.left);
                }
            }
        }
    }

    TEST_METHOD(TestGetSelectionRects_LineMode)
    {
        {
            auto selection{ m_pSelection->_d.write() };
            selection->fSelectionVisible = true;

            // Part I: Multiple line selection
            // set selection region
            selection->srSelectionRect.top = 0;
            selection->srSelectionRect.bottom = 3;
            selection->srSelectionRect.left = 1;
            selection->srSelectionRect.right = 10;

            // #1 top-left to bottom right selection first
            selection->coordSelectionAnchor.x = selection->srSelectionRect.left;
            selection->coordSelectionAnchor.y = selection->srSelectionRect.top;

            // A. true/false for the selection modes should mean line selection
            selection->fLineSelection = true;
            selection->fUseAlternateSelection = false;

            VerifyGetSelectionRects_LineMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // B. false/true for the selection modes should also mean line selection
            selection->fLineSelection = false;
            selection->fUseAlternateSelection = true;

            VerifyGetSelectionRects_LineMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // now try the other 3 configurations of box region.
            // #2 top-right to bottom-left selection
            selection->coordSelectionAnchor.x = selection->srSelectionRect.right;
            selection->coordSelectionAnchor.y = selection->srSelectionRect.top;

            VerifyGetSelectionRects_LineMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // #3 bottom-left to top-right selection
            selection->coordSelectionAnchor.x = selection->srSelectionRect.left;
            selection->coordSelectionAnchor.y = selection->srSelectionRect.bottom;

            VerifyGetSelectionRects_LineMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // #4 bottom-right to top-left selection
            selection->coordSelectionAnchor.x = selection->srSelectionRect.right;
            selection->coordSelectionAnchor.y = selection->srSelectionRect.bottom;

            VerifyGetSelectionRects_LineMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // Part II: Single line selection
            selection->srSelectionRect.top = 2;
            selection->srSelectionRect.bottom = 2;
            selection->srSelectionRect.left = 1;
            selection->srSelectionRect.right = 10;

            // #1: left to right selection
            selection->coordSelectionAnchor.x = selection->srSelectionRect.left;
            VERIFY_IS_TRUE(selection->srSelectionRect.bottom == selection->srSelectionRect.top);
            selection->coordSelectionAnchor.y = selection->srSelectionRect.bottom;

            VerifyGetSelectionRects_LineMode();
        }

        {
            auto selection{ m_pSelection->_d.write() };
            // #2: right to left selection
            selection->coordSelectionAnchor.x = selection->srSelectionRect.right;
            VERIFY_IS_TRUE(selection->srSelectionRect.bottom == selection->srSelectionRect.top);
            selection->coordSelectionAnchor.y = selection->srSelectionRect.top;

            VerifyGetSelectionRects_LineMode();
        }
    }

    void TestBisectSelectionDelta(til::CoordType sTargetX, til::CoordType sTargetY, til::CoordType sLength, til::CoordType sDeltaLeft, til::CoordType sDeltaRight)
    {
        const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        const auto& screenInfo = gci.GetActiveOutputBuffer();

        til::CoordType sStringLength;
        til::point coordTargetPoint;
        til::inclusive_rect srSelection;
        til::inclusive_rect srOriginal;

        sStringLength = sLength;
        coordTargetPoint.x = sTargetX;
        coordTargetPoint.y = sTargetY;

        // selection area is always one row at a time so top/bottom = Y = row position
        srSelection.top = srSelection.bottom = coordTargetPoint.y;

        // selection rectangle starts from the target and goes for the length requested
        srSelection.left = coordTargetPoint.x;
        srSelection.right = coordTargetPoint.x + sStringLength;

        // save original for comparison
        srOriginal.top = srSelection.top;
        srOriginal.bottom = srSelection.bottom;
        srOriginal.left = srSelection.left;
        srOriginal.right = srSelection.right;

        til::point startPos{ sTargetX, sTargetY };
        til::point endPos{ sTargetX + sLength, sTargetY };
        const auto selectionRects = screenInfo.GetTextBuffer().GetTextRects(startPos, endPos, false, false);

        VERIFY_ARE_EQUAL(static_cast<size_t>(1), selectionRects.size());
        srSelection = selectionRects.at(0);

        VERIFY_ARE_EQUAL(srOriginal.top, srSelection.top);
        VERIFY_ARE_EQUAL(srOriginal.bottom, srSelection.bottom);
        VERIFY_ARE_EQUAL(srOriginal.left + sDeltaLeft, srSelection.left);
        VERIFY_ARE_EQUAL(srOriginal.right + sDeltaRight, srSelection.right);
    }
};

class SelectionInputTests
{
    TEST_CLASS(SelectionInputTests);

    CommonState* m_state;
    CommandHistory* m_pHistory;

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_state = new CommonState();

        m_state->PrepareGlobalInputBuffer();
        m_state->PrepareGlobalScreenBuffer();
        m_pHistory = CommandHistory::s_Allocate(L"cmd.exe", nullptr);
        if (!m_pHistory)
        {
            return false;
        }
        // History must be prepared before COOKED_READ (as it uses s_Find to get at it)

        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        CommandHistory::s_Free(nullptr);
        m_pHistory = nullptr;
        m_state->CleanupGlobalScreenBuffer();
        m_state->CleanupGlobalInputBuffer();

        delete m_state;

        return true;
    }

    TEST_METHOD(TestWordByWordPrevious)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
        END_TEST_METHOD_PROPERTIES()

        auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        auto& screenInfo = gci.GetActiveOutputBuffer();

        const std::wstring text(L"this is some test text.");
        screenInfo.Write(OutputCellIterator(text));

        // Get the left and right side of the text we inserted (right is one past the end)
        const til::point left;
        const til::point right{ gsl::narrow<til::CoordType>(text.length()), 0 };

        // Get the selection instance and buffer size
        auto& sel = Selection::Instance();
        const auto bufferSize = screenInfo.GetBufferSize();

        // The anchor is where the selection started from.
        const auto anchor = right;

        // The point is the "other end" of the anchor forming the rectangle of what is covered.
        // It starts at the same spot as the anchor to represent the initial 1x1 selection.
        auto point = anchor;

        // Walk through the sequence in reverse extending the sequence by one word each time to the left.
        // The anchor is always the end of the line and the selection just gets bigger.
        do
        {
            // We expect the result to be left of where we started.
            // It will point at the character just right of the space (or the beginning of the line).
            auto resultExpected = point;

            do
            {
                resultExpected.x--;
            } while (resultExpected.x > 0 && text.at(resultExpected.x - 1) != UNICODE_SPACE);

            point = sel.WordByWordSelection(true, bufferSize, anchor, point);

            VERIFY_ARE_EQUAL(resultExpected, point);

        } while (point.x > left.x);
    }

    TEST_METHOD(TestWordByWordNext)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
        END_TEST_METHOD_PROPERTIES()

        auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        auto& screenInfo = gci.GetActiveOutputBuffer();

        const std::wstring text(L"this is some test text.");
        screenInfo.Write(OutputCellIterator(text));

        // Get the left and right side of the text we inserted (right is one past the end)
        const til::point left;
        const til::point right = { gsl::narrow<til::CoordType>(text.length()), 0 };

        // Get the selection instance and buffer size
        auto& sel = Selection::Instance();
        const auto bufferSize = screenInfo.GetBufferSize();

        // The anchor is where the selection started from.
        const auto anchor = left;

        // The point is the "other end" of the anchor forming the rectangle of what is covered.
        // It starts at the same spot as the anchor to represent the initial 1x1 selection.
        auto point = anchor;

        // Walk through the sequence forward extending the sequence by one word each time to the right.
        // The anchor is always the end of the line and the selection just gets bigger.
        do
        {
            // We expect the result to be right of where we started.

            auto resultExpected = point;

            do
            {
                resultExpected.x++;
            } while (resultExpected.x + 1 < right.x && text.at(resultExpected.x + 1) != UNICODE_SPACE);
            resultExpected.x++;

            // when we reach the end, word by word selection will seek forward to the end of the buffer, so update
            // the expected to the end in that circumstance
            if (resultExpected.x >= right.x)
            {
                resultExpected.x = bufferSize.RightInclusive();
                resultExpected.y = bufferSize.BottomInclusive();
            }

            point = sel.WordByWordSelection(false, bufferSize, anchor, point);

            VERIFY_ARE_EQUAL(resultExpected, point);

        } while (point.y < bufferSize.BottomInclusive()); // stop once we've advanced to a point on the bottom row of the buffer.
    }
};
