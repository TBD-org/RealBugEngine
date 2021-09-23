#pragma once

#include "Scripting/Script.h"
#include "Player.h"

class ComponentText;
class GameObject;
class PlayerController;
class CameraController;

class DialogueManager : public Script
{
	GENERATE_BODY(DialogueManager);

public:
	enum class DialogueWindow {
		NONE = 0,
		FANG = 1,
		ONIMARU = 2,
		DUKE = 3,
		DOOR = 4,
		TUTO_FANG = 5,
		TUTO_FANG_ULTI = 6,
		TUTO_ONIMARU = 7,
		TUTO_ONIMARU_ULTI = 8,
		TUTO_SWAP = 9,
		UPGRADES1 = 10,
		UPGRADES2 = 11,
		UPGRADES3 = 12
	};

	struct Dialogue {
		Dialogue() {};
		Dialogue(DialogueWindow character_, bool isBlocking_, const char* text_, Dialogue* nextDialogue_, InputActions closeButton_ = InputActions::INTERACT) : character(character_), isBlocking(isBlocking_), text(text_), nextDialogue(nextDialogue_), closeButton(closeButton_) {};

		DialogueWindow character = DialogueWindow::NONE;	// Indirect reference to which dialogue window gameObject must be opened.
		const char* text = nullptr;							// Text shown in the dialogue (only for 1,2,3 & 4. Should be empty for Tutorials and Upgrades).
		Dialogue* nextDialogue = nullptr;					// Pointer to the next Dialogue in 'dialoguesArray', that will come out after this one.
		bool isBlocking = false;							// Whether or not this dialogue window should pause the gameplay.
		InputActions closeButton = InputActions::INTERACT;	// Definition of the button that will close this dialogue.
	};

	enum class AudioDialogue {
		OPEN,
		SWOOSH,
		FLASH,
		BUTTON,
		TOTAL
	};

public:

	void Start() override;
	void Update() override;

	void SetActiveDialogue(Dialogue* dialogue, bool runAnimation = true); // Sets the dialogue window in 'dialoguesArray' that must be activated next. Stops the dialogue flow if 'dialogue = nullptr".

	void ActivateDialogue(Dialogue* dialogue);		// Starts the position and color "Open transition" for the active dialogue.
	void ActivateNextDialogue(Dialogue* dialogue);	// Advances to the next linked dialogue, and determines if transitions must be done for the two dialogue windows. (It will activate Open and Close transitions if the windows are of different type)
	void CloseDialogue(Dialogue* dialogue);			// Starts the position and color "Close transition" for the active dialogue.
	void ActivatePowerUpDialogue();					// Calls the specific 'SetActiveDialogue()' depending on the number of upgrades collected in the level.

	void TransitionUIElementsColor(bool appearing = true, bool mustLerp = true);	// Subfunction of 'ActivateDialogue()'. Defines the color transitions of the dialogue window.
	void RetrieveUIComponents(GameObject* current);									// This function gets all UI components of the active dialogue window. The color transitions will be aplied to all of them.
	bool HasActiveDialogue();

	void PlayOpeningAudio();
public:
	Dialogue dialoguesArray[100];					// Contains the definition and characteristics of every dialogue in the game.

	// ------ GAMEOBJECT REFS ------ //
	UID fangTextObjectUID = 0;
	UID onimaruTextObjectUID = 0;
	UID dukeTextObjectUID = 0;
	UID doorTextObjectUID = 0;
	ComponentText* fangTextComponent = nullptr;
	ComponentText* onimaruTextComponent = nullptr;
	ComponentText* dukeTextComponent = nullptr;
	ComponentText* doorTextComponent = nullptr;

	UID tutorialFangUID = 0;
	UID tutorialFangUltimateUID = 0;
	UID tutorialOnimaruUID = 0;
	UID tutorialOnimaruUltimateUID = 0;
	UID tutorialSwapUID = 0;
	GameObject* tutorialFang = nullptr;
	GameObject* tutorialFangUltimate = nullptr;
	GameObject* tutorialOnimaru = nullptr;
	GameObject* tutorialOnimaruUltimate = nullptr;
	GameObject* tutorialSwap = nullptr;

	UID tutorialUpgrades1UID = 0;
	UID tutorialUpgrades2UID = 0;
	UID tutorialUpgrades3UID = 0;
	GameObject* tutorialUpgrades1 = nullptr;
	GameObject* tutorialUpgrades2 = nullptr;
	GameObject* tutorialUpgrades3 = nullptr;

	UID playerUID = 0;
	GameObject* player = nullptr;
	PlayerController* playerControllerScript = nullptr;

	UID gameCameraUID = 0;
	GameObject* camera = nullptr;
	CameraController* cameraControllerScript = nullptr;

	// ----- TRANSITION CONFIG ----- //
	float3 dialogueStartPosition = float3(0, 0, 0); // "Closed" position of the dialogue windows.
	float3 dialogueEndPosition = float3(0, 0, 0);	// "Open" position of the dialogue windows.
	float3 tutorialStartPosition = float3(0, 0, 0);	// "Closed" position of the tutorial and upgrades windows.
	float3 tutorialEndPosition = float3(0, 0, 0);	// "Open" position of the tutorial and upgrades windows.
	float appearAnimationTime = .5f;				// Duration time of the "Open transition".
	float disappearAnimationTime = .5f;				// Duration time of the "Close transition".
	float3 newCameraPosition = float3(0, 0, 0);		// Zoomed position of the camera when a dialogue is opened.

	// ------ FLASH TRANSITION ----- //
	UID flashUID = 0;
	GameObject* flash = nullptr;					// The flash effect that appears between consecutives dialgues of different characters.
	float flashTime = 0.1f;							// The duration of the flash effect.

	// ---------- AUDIOS ----------- //
	UID audioSourcesUID = 0;
	GameObject* audioSources = nullptr;
	ComponentAudioSource* audios[static_cast<int>(AudioDialogue::TOTAL)] = { nullptr };

private:
	Dialogue* activeDialogue = nullptr;				// Pointer to dialoguesArray of the active dialogue.
	GameObject* activeDialogueObject = nullptr;		// Pointer to the Dialogue GameObject in the scene (DialogueFang, DialogueOnimaru, DialogueDuke, Tutorials or Upgrades).
	int obtainedPowerUps = 0;						// PowerUp counter. Increments each time the player picks up a powerUp.

	// ------ TRANSITION INFO ------ //
	float3 currentStartPosition = float3(0, 0, 0);	// Captures wether 'dialogueStartPosition' or 'tutorialStartPosition', that must be used fot the current dialogue window.
	float3 currentEndPosition = float3(0, 0, 0);	// Captures wether 'dialogueEndPosition' or 'tutorialEndPosition', that must be used fot the current dialogue window.
	float animationLerpTime = 0.0f;					// Stores the elapsed time during Open and Close transitions.
	float elapsedFlashTime = 0.f;					// Stores the elapsed time of the flash effect.
	std::vector<Component*> uiComponents;			// Stores the UI components in the active dialogue window. Obtained from 'RetrieveUIComponents()'. Used in 'TransitionUIElementsColor()'.
	std::vector<float4> uiColors;					// Stores the corresponding color of each 'uiComponents'.

	// ---- TRANSITION TRIGGERS ---- //
	bool runOpenAnimation = false;
	bool runChangeAnimation = false;
	bool runCloseAnimation = false;
	bool runSecondaryOpen = false;
	bool mustFlash = false;
	bool triggerAudio = true;
};
