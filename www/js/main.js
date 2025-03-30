// 导航栏滚动效果
window.addEventListener('scroll', function() {
    const header = document.querySelector('.header');
    if (window.scrollY > 50) {
        header.style.boxShadow = '0 2px 10px rgba(0, 0, 0, 0.2)';
    } else {
        header.style.boxShadow = '0 2px 10px rgba(0, 0, 0, 0.1)';
    }
});

// 简单的轮播图功能
document.addEventListener('DOMContentLoaded', function() {
    // 这里可以添加轮播图逻辑
    console.log('网站加载完成');
});

// 响应式菜单切换
document.querySelector('.menu-toggle').addEventListener('click', function() {
    const nav = document.querySelector('.nav ul');
    nav.classList.toggle('active');
});

// 产品项点击效果
const productItems = document.querySelectorAll('.product-item');
productItems.forEach(item => {
    item.addEventListener('click', function() {
        // 这里可以添加产品详情页跳转逻辑
        console.log('点击了产品:', this.querySelector('h3').textContent);
    });
});
