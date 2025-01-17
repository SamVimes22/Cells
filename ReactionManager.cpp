#include "ReactionManager.h"
#include "Utilities.h"

int AttemptReaction(std::vector<Compound>& initial, int threshold, std::vector<Compound*>& reactants)
{
    if (initial.size() != 0)
    {
        int idx1 = rand() % initial.size();
        int idx2 = rand() % initial.size();
        //a tenth of the time, just try to react with yourself (i.e. break apart or move things around)
        if (rand() % 10 == 0)
        {
            idx2 = idx1;
        }
        Compound& comp1 = initial[idx1];
        Compound& comp2 = initial[idx2];
        int stability = 0;
        int ielem1;
        int ielem2;
        if (PerformReactionIfGoodEnough(&comp1, &comp2, threshold, stability, ielem1, ielem2))
        {
            //if comp1 was cleared out
            if (comp1.mass == 0)
            {
                //if comp2 is at the end (and therefore about to be moved by fastdelete)
                if (idx2 == initial.size() - 1)
                {
                    FastDelete(initial, idx1);
                    reactants.emplace_back(&comp1); //confusing, but comp1 is atually comp2 now, so this works correctly
                }
                else
                {
                    reactants.emplace_back(&comp2);
                    FastDelete(initial, idx1);
                }
            }
            else
            {
                int numpieces = 0;
                Compound* pieces = comp1.SplitCompound(ielem1, numpieces);
                if (pieces == nullptr)
                {
                    //there was no split, the reactants are still the same two compounds we started with
                    reactants.emplace_back(&comp1);
                    if (idx1 != idx2)
                    {
                        reactants.emplace_back(&comp2);
                    }
                }
                else
                {
                    //there was a split, we need to do some fancy things to keep our pointers and references happy
                    Compound temp(comp2);//copy comp2, which needs to get back into the initial solution
                    FastDelete(initial, idx2); //get rid of the thing we just copied

                    //this is incorrect if theres only one compound
                    if (idx1 != idx2)
                    {
                        //get rid of the broken up part
                        if (idx1 == initial.size())//if idx1 was at the end, the previous fastdelete moved it
                        {
                            FastDelete(initial, idx2); //this is where it is now
                        }
                        else
                        {
                            FastDelete(initial, idx1); //the normal execution
                        }
                        initial.emplace_back(temp); //add comp2 back in expected location
                    }

                    //now, we can add things back and make references to those things for the reactants list
                    for (int i = 0; i < numpieces; i++)
                    {
                        initial.emplace_back(pieces[i]);
                    }
                    delete[] pieces;
                    //if there were two compounds to begin with, put the extant one into the reactants list
                    if (idx1 != idx2)
                    {
                        numpieces += 1;
                    }
                    for (int i = numpieces; i > 0; --i)
                    {
                        reactants.emplace_back(&initial[initial.size() - i]);
                    }
                    //now, we have all the compounds added that should be, and the references were 
                    //all safely made after inserting things into the initial vector
                }
            }
            return stability;
        }

    }
    return -1;
}

//NOTE: deleted compounds dont show up here.
//we are going to lose a lot of energy to compounds with internal energy that just get deleted
void AdjustEnergyValues(int stabilityChange, std::vector<Compound*>& reactants)
{
    if (reactants.size() != 0)
    {
        int energyToTransfer = stabilityChange / reactants.size();
        for (Compound* c : reactants)
        {
            c->internalEnergy += energyToTransfer;
        }
        reactants[0]->internalEnergy += stabilityChange - energyToTransfer * reactants.size(); //leftover after int. division
    }
}

bool PerformReactionIfGoodEnough(Compound* comp1, Compound* comp2, int threshold, int& stabilityChange, int& ielem1, int& ielem2)
{
    int idxidx1 = rand() % comp1->filledIndices.size();
    int idxidx2 = rand() % comp2->filledIndices.size();
    ielem1 = comp1->filledIndices[idxidx1];
    ielem2 = comp2->filledIndices[idxidx2];
    int emptyIdxs[6];
    int spaces = comp2->getUnPopulatedNeighborsIndices(ielem2, emptyIdxs);
    if (spaces == 0)
    {
        return false;
    }
    ielem2 = emptyIdxs[rand() % spaces];

    // current instability
    int stability = 0;
    if (comp1 != comp2)
        //stability = comp1.GetTotalStability() + comp2.GetTotalStability();
        stability = comp1->GetStabilityAtPoint(ielem1) + comp2->GetStabilityAtPoint(ielem2);
    else
        stability = comp1->GetTotalStability();
    DoReaction(*comp1, *comp2, ielem1, ielem2, idxidx1);
    // subtract the instability of the new, so if the result is positive we have increased stability
    if (comp1 != comp2)
        //stability = comp1.GetTotalStability() + comp2.GetTotalStability() - stability;
        stability = (comp1->GetStabilityAtPoint(ielem1) + comp2->GetStabilityAtPoint(ielem2)) - stability;
    else
        stability = comp1->GetTotalStability() - stability;

    if (stability < threshold)
    {
        //undo the reaction
        DoReaction(*comp2, *comp1, ielem2, ielem1, comp2->filledIndices.size() - 1);
        return false;
    }
    return true;
}


//may eventually be changed to actually traverse the structure of a molecule and find a location on the outside.
//however, the number of cycles that this would require to complete may simply make this untenable
//fast and bad may be better than good but really slow? the goal is complex behaviour, if going faster lets us
//achieve more progress, im all about it
int FindElementIdxByFalling(Compound& c)
{
    int ret;
    while (c.composition[c.filledIndices[(ret = rand() % c.filledIndices.size())]].red == 0)
    {
        FastDelete(c.filledIndices, ret);
    }
    ret = c.filledIndices[ret];
    return ret;
}

void DoReaction(Compound& c1, Compound& c2, int idx1, int idx2, int idxidxtoremove)
{
    Element e = c1.composition[idx1];
    c1.RemoveElementAtIndex(idx1);
    FastDelete(c1.filledIndices, idxidxtoremove);
    c2.AddElementAtIndex(e, idx2);
}