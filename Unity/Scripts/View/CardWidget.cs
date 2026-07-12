using UnityEngine;
using UnityEngine.UI;
using TMPro; // TextMeshPro 支持中文显示

/// <summary>
/// 卡牌控件 — 单张卡牌的 UI 表现
/// 挂载到 Card 预制体上
/// </summary>
public class CardWidget : MonoBehaviour
{
    [Header("UI 组件")]
    [SerializeField] private TextMeshProUGUI _suitText;    // 花色符号 ♠♥♣♦
    [SerializeField] private TextMeshProUGUI _numberText;   // 点数 A-10 J Q K
    [SerializeField] private TextMeshProUGUI _nameText;     // 卡名 "杀"
    [SerializeField] private TextMeshProUGUI _descText;     // 描述（可选）

    [Header("背景")]
    [SerializeField] private Image _cardBg;                 // 卡牌底色
    [SerializeField] private Color _redColor = new Color(0.9f, 0.2f, 0.2f, 1f);
    [SerializeField] private Color _blackColor = new Color(0.2f, 0.2f, 0.2f, 1f);

    [Header("状态")]
    [SerializeField] private GameObject _selectedMark;      // 选中标记
    [SerializeField] private GameObject _playableGlow;      // 可打出发光
    [SerializeField] private GameObject _unplayableMask;    // 不可打出遮罩
    [SerializeField] private Button _button;

    private CardViewModel _viewModel;
    public CardViewModel ViewModel => _viewModel;

    private System.Action<CardWidget> _onClicked;

    /// <summary>绑定 ViewModel</summary>
    public void Bind(CardViewModel vm, System.Action<CardWidget> onClicked = null)
    {
        _viewModel = vm;
        _onClicked = onClicked;

        if (vm == null) return;

        Refresh();

        // 监听 ViewModel 状态变化
        vm.SelectedChanged.Connect(OnSelectedChanged);
        vm.PlayableChanged.Connect(OnPlayableChanged);
    }

    private void OnDestroy()
    {
        // C# GC 会处理，但显式断开好习惯
    }

    private void OnEnable()
    {
        if (_button != null)
            _button.onClick.AddListener(OnClick);
    }

    private void OnDisable()
    {
        if (_button != null)
            _button.onClick.RemoveListener(OnClick);
    }

    private void OnClick()
    {
        _onClicked?.Invoke(this);
    }

    /// <summary>刷新所有 UI</summary>
    public void Refresh()
    {
        if (_viewModel == null) return;

        // 花色 + 颜色
        if (_suitText != null)
        {
            _suitText.text = _viewModel.SuitSymbol();
            _suitText.color = _viewModel.Color == CardColor.Red ? _redColor : _blackColor;
        }

        if (_numberText != null)
        {
            _numberText.text = _viewModel.NumberString();
            _numberText.color = _viewModel.Color == CardColor.Red ? _redColor : _blackColor;
        }

        if (_nameText != null)
            _nameText.text = _viewModel.CardName;

        if (_cardBg != null)
            _cardBg.color = _viewModel.Color == CardColor.Red ? _redColor : _blackColor;
    }

    private void OnSelectedChanged(bool selected)
    {
        // 向上移动表示选中
        if (_selectedMark != null)
            _selectedMark.SetActive(selected);

        transform.localPosition = selected
            ? new Vector3(transform.localPosition.x, 20f, transform.localPosition.z)
            : new Vector3(transform.localPosition.x, 0f, transform.localPosition.z);
    }

    private void OnPlayableChanged(bool playable)
    {
        if (_playableGlow != null)
            _playableGlow.SetActive(playable);

        if (_unplayableMask != null)
            _unplayableMask.SetActive(!playable);

        if (_button != null)
            _button.interactable = playable;
    }
}
