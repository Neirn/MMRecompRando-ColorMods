#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "globalobjects_api.h"
#include "playermodelmanager_api.h"
#include "dl_patching.h"
#include "compat_pmm.h"
#include "recolor.h"
#include "models.h"

bool is_player_model_manager_loaded = false;
bool is_global_objects_loaded = false;

RECOMP_CALLBACK("*", recomp_on_init)
void check_if_optional_pmm_dependencies_loaded() {
    is_global_objects_loaded = recomp_is_dependency_met(YAZMT_Z64_GLOBAL_OBJECTS_MOD_NAME) == DEPENDENCY_STATUS_FOUND;
    is_player_model_manager_loaded = recomp_is_dependency_met(YAZMT_PMM_MOD_NAME) == DEPENDENCY_STATUS_FOUND && is_global_objects_loaded;
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
        void *obj = GlobalObjects_getGlobalObject(forms_information[i].id);

        if (obj) {
            GlobalObjects_globalizeLodLimbSkeleton(obj, forms_information[i].skel);
            // patch_link_skeleton_dls(SEGMENTED_TO_GLOBAL_PTR(obj, forms_information[i].skel), i);
        }
    }
}

static struct {
    void *dramAddr;
    uintptr_t vromAddr;
    size_t size;
} dma_request_info;

RECOMP_HOOK("DmaMgr_ProcessRequest")
void patch_colors_on_DmaMgr_ProcessRequest(DmaRequest *req) {
    dma_request_info.dramAddr = req->dramAddr;
    dma_request_info.vromAddr = req->vromAddr;
    dma_request_info.size = req->size;
}

RECOMP_HOOK_RETURN("DmaMgr_ProcessRequest")
void patch_colors_on_return_DmaMgr_ProcessRequest() {
    if (is_global_objects_loaded) {
        ObjectId id;

        if (GlobalObjects_getObjectIdFromVrom(dma_request_info.vromAddr, &id)) {
            PlayerTransformation form;

            void (*replace_func)(Gfx *, s32) = NULL;

            switch (id) {
                case GAMEPLAY_KEEP: {
                    Gfx *toPatch = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, gameplay_keep_DL_06FE20);
                    patch_prim_color_with_dl(toPatch, LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[PLAYER_FORM_ZORA][0]);
                    replace_zora_boomerang(toPatch, 0);

                    toPatch = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, gameplay_keep_DL_06FF68);
                    patch_prim_color_with_dl(toPatch, LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[PLAYER_FORM_ZORA][0]);
                    replace_zora_boomerang(toPatch, 1);
                    break;
                }

                case OBJECT_LINK_BOY:
                    replace_func = replace_fd;
                    form = PLAYER_FORM_FIERCE_DEITY;
                    break;

                case OBJECT_LINK_GORON: {
                    replace_func = replace_goron;
                    form = PLAYER_FORM_GORON;

                    Gfx *toPatch = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, gLinkGoronCurledDL);
                    patch_prim_color_with_dl(toPatch, LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[PLAYER_FORM_GORON][0]);
                    replace_goron_roll(toPatch);
                    break;
                }

                case OBJECT_LINK_ZORA: {
                    Gfx *toPatch = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, object_link_zora_DL_00CC38);
                    patch_prim_color_with_dl(toPatch, LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[PLAYER_FORM_ZORA][0]);
                    replace_zora_fins(toPatch, 0);

                    toPatch = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, object_link_zora_DL_00CDA0);
                    patch_prim_color_with_dl(toPatch, LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[PLAYER_FORM_ZORA][0]);
                    replace_zora_fins(toPatch, 1);

                    toPatch = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, object_link_zora_DL_010868);
                    patch_prim_color_with_dl(toPatch, LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[PLAYER_FORM_ZORA][0]);
                    replace_zora_fins(toPatch, 2);

                    toPatch = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, object_link_zora_DL_010978);
                    patch_prim_color_with_dl(toPatch, LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[PLAYER_FORM_ZORA][0]);
                    replace_zora_fins(toPatch, 3);

                    toPatch = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, object_link_zora_DL_0110A8);
                    patch_prim_color_with_dl(toPatch, LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[PLAYER_FORM_ZORA][0]);
                    replace_zora_fins(toPatch, 4);

                    replace_func = replace_zora;
                    form = PLAYER_FORM_ZORA;
                    break;
                }

                case OBJECT_LINK_NUTS: {
                    replace_func = replace_deku;
                    form = PLAYER_FORM_DEKU;
                    break;
                }

                case OBJECT_LINK_CHILD: {
                    form = PLAYER_FORM_HUMAN;
                    break;
                }

                default:
                    return;
                    break;
            }

            if (replace_func) {
                FlexSkeletonHeader *skel = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, forms_information[form].skel);
                void **limbs = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, skel->sh.segment);

                for (s32 i = 1; i < PLAYER_LIMB_MAX; ++i) {
                    LodLimb *limb = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, limbs[i - 1]);
                    Gfx *toPatch = SEGMENTED_TO_GLOBAL_PTR(dma_request_info.dramAddr, limb->dLists[0]);

                    if (toPatch) {
                        patch_prim_color_with_dl(toPatch, LINK_R, LINK_G, LINK_B, LINK_A, &prim_color_dls[form][0]);

                        replace_func(toPatch, i);
                    }
                }
            }
        }
    }
}
