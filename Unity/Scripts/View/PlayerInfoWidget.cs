using UnityEngine;
using UnityEngine.UI;
using TMPro;

/// <summary>
/// 玩家信息控件 — 显示体力、武将名、手牌数等
/// </summary>
public class PlayerInfoWidget : MonoBehaviour
{
    [Header("UI 组件")]
    [SerializeField] private TextMeshProUGUI _nameText;
    [SerializeField] private TextMeshProUGUI _characterText;
    [SerializeField] private TextMeshProUGUI _skillText;
    [SerializeField] private Slider _hpSlider;
    [SerializeField] private TextMeshProUGUI _hpText;
    [SerializeField] private TextMeshProUGUI _cardCountText;
    [SerializeField] private GameObject _turnHighlight;     // 当前回合高亮

    private PlayerViewModel _viewModel;
    public PlayerViewModel ViewModel => _viewModel;

    /// <summary>绑定 ViewModel</summary>
    public void Bind(PlayerViewModel vm)
    {
        if (_viewModel != null)
            _viewModel.Dispose();

        _viewModel = vm;

        if (vm == null) return;

        vm.HpChanged.Connect(_ => Refresh());
        vm.MaxHpChanged.Connect(_ => Refresh());
        vm.HandCardsChanged.Connect(() => Refresh());
        vm.StateChanged.Connect(() => Refresh());
        vm.Died.Connect(_ => OnDied());

        Refresh();
    }

    private void OnDestroy()
    {
        _viewModel?.Dispose();
    }

    public void SetTurnHighlight(bool active)
    {
        if (_turnHighlight != null)
            _turnHighlight.SetActive(active);
    }

    public void Refresh()
    {
        if (_viewModel == null) return;

        if (_nameText != null)
            _nameText.text = _viewModel.DisplayName;

        if (_characterText != null)
            _characterText.text = $"{_viewModel.CharacterName} 【{_viewModel.SkillName}】";

        if (_skillText != null)
            _skillText.text = _viewModel.SkillDescription;

        if (_hpSlider != null)
        {
            _hpSlider.maxValue = _viewModel.MaxHp;
            _hpSlider.value = _viewModel.Hp;
        }

        if (_hpText != null)
            _hpText.text = $"{_viewModel.Hp}/{_viewModel.MaxHp}";

        if (_cardCountText != null)
            _cardCountText.text = $"手牌: {_viewModel.HandCardCount}";
    }

    private void OnDied()
    {
        // 阵亡视觉反馈
        var canvasGroup = GetComponent<CanvasGroup>();
        if (canvasGroup != null)
            canvasGroup.alpha = 0.4f;

        if (_hpText != null)
            _hpText.text = "阵亡";
    }
}
