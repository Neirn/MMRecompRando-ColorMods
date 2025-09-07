#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "globalobjects_api.h"
#include "playermodelmanager_api.h"
#include "dl_patching.h"
#include "compat_pmm.h"

bool is_player_model_manager_loaded = false;

RECOMP_CALLBACK("*", recomp_on_init) void check_if_player_model_manager_loaded() {
    is_player_model_manager_loaded = recomp_is_dependency_met(YAZMT_PMM_MOD_NAME) == DEPENDENCY_STATUS_FOUND && recomp_is_dependency_met(YAZMT_Z64_GLOBAL_OBJECTS_MOD_NAME) == DEPENDENCY_STATUS_FOUND;
}

#define LINK_R 30
#define LINK_G 105
#define LINK_B 27
#define LINK_A 255

extern Gfx prim_color_dls[PLAYER_FORM_MAX][2];

static void patch_link_skeleton_dls(FlexSkeletonHeader *skel, PlayerTransformation transformation) {
    for (int i = 0; i < skel->sh.limbCount; ++i) {
        LodLimb *limb = skel->sh.segment[i];

        if (limb->dLists[0]) {
            patch_prim_color_with_dl(limb->dLists[0], LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[transformation][0]);
        }

        if (limb->dLists[1]) {
            patch_prim_color_with_dl(limb->dLists[1], LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[transformation][0]);
        }
    }
}

extern FlexSkeletonHeader gLinkHumanSkel;
extern FlexSkeletonHeader gLinkDekuSkel;
extern FlexSkeletonHeader gLinkGoronSkel;
extern FlexSkeletonHeader gLinkZoraSkel;
extern FlexSkeletonHeader gLinkFierceDeitySkel;

typedef struct {
    ObjectId id;
    FlexSkeletonHeader *skel;
} FormInfo;

static const FormInfo forms_information[PLAYER_FORM_MAX] = {
    {.id = OBJECT_LINK_BOY, .skel = &gLinkFierceDeitySkel}, // PLAYER_FORM_FIERCE_DEITY
    {.id = OBJECT_LINK_GORON, .skel = &gLinkGoronSkel},     // PLAYER_FORM_GORON
    {.id = OBJECT_LINK_ZORA, .skel = &gLinkZoraSkel},       // PLAYER_FORM_ZORA
    {.id = OBJECT_LINK_NUTS, .skel = &gLinkDekuSkel},       // PLAYER_FORM_DEKU
    {.id = OBJECT_LINK_CHILD, .skel = &gLinkHumanSkel},     // PLAYER_FORM_HUMAN
};

GLOBAL_OBJECTS_CALLBACK_ON_READY void on_global_objects_loaded() {
    for (PlayerTransformation i = 0; i < PLAYER_FORM_MAX; ++i) {
        void* obj = GlobalObjects_getGlobalObject(forms_information[i].id);

        if (obj) {
            GlobalObjects_globalizeLodLimbSkeleton(obj, forms_information[i].skel);
            patch_link_skeleton_dls(SEGMENTED_TO_GLOBAL_PTR(obj, forms_information[i].skel), i);
        }
    }
}
