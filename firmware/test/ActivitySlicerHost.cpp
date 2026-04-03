#include <stdint.h>
#include <stdio.h>

#include "../lib/Modem/ActivitySlicer.h"

using TestSlicer = ActivitySlicer<30, 12, 8, 24>;

static bool testIdleStaysSilent()
{
    TestSlicer slicer;
    const uint16_t noise[] = {3, 5, 4, 2, 6, 4, 3, 5, 4, 2, 5, 3, 4, 6, 3, 2};

    for (uint8_t i = 0; i < sizeof(noise) / sizeof(noise[0]); ++i)
    {
        if (slicer.update(noise[i]) != TestSlicer::STATE_NONE)
        {
            fprintf(stderr, "noise sequence should not classify as tone\n");
            return false;
        }
    }

    if (slicer.tonePresent())
    {
        fprintf(stderr, "noise sequence should not trip tone detection\n");
        return false;
    }

    return true;
}

static bool testStrongToneProducesTransitions()
{
    TestSlicer slicer;
    const uint16_t tone[] = {240, 330, 445, 525, 455, 335, 225, 315, 430, 515, 450, 320};

    TestSlicer::State prev = TestSlicer::STATE_NONE;
    uint16_t transitions = 0;

    for (uint8_t repeat = 0; repeat < 8; ++repeat)
    {
        for (uint8_t i = 0; i < sizeof(tone) / sizeof(tone[0]); ++i)
        {
            TestSlicer::State next = slicer.update(tone[i]);
            if (prev != TestSlicer::STATE_NONE && next != TestSlicer::STATE_NONE && next != prev)
            {
                transitions++;
            }
            if (next != TestSlicer::STATE_NONE)
            {
                prev = next;
            }
        }
    }

    if (!slicer.tonePresent())
    {
        fprintf(stderr, "strong tone should be reported as present\n");
        return false;
    }

    if (transitions < 8)
    {
        fprintf(stderr, "expected adaptive slicer to produce transitions, saw %u\n", transitions);
        return false;
    }

    return true;
}

int main()
{
    if (!testIdleStaysSilent())
    {
        return 1;
    }
    if (!testStrongToneProducesTransitions())
    {
        return 1;
    }
    return 0;
}
