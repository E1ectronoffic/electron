import { EventEmitter } from 'events';

let _inAppPurchase;

// See: https://developer.apple.com/documentation/storekit/skpaymentdiscount
type PaymentDiscount = {
  identifier: string;
  keyIdentifier: string;
  nonce: string;
  signature: string;
  timestamp: number;
};

if (process.platform === 'darwin') {
  const { inAppPurchase } = process._linkedBinding('electron_browser_in_app_purchase');
  const _purchase = inAppPurchase.purchaseProduct as (productID: string, quantity?: number, username?: string, paymentDiscount?: PaymentDiscount) => Promise<boolean>;
  inAppPurchase.purchaseProduct = (productID: string, opts?: number | { quantity?: number, username?: string, paymentDiscount?: PaymentDiscount }) => {
    const quantity = typeof opts === 'object' ? opts.quantity : opts;
    const username = typeof opts === 'object' ? opts.username : undefined;
    const paymentDiscount = typeof opts === 'object' ? opts.paymentDiscount : undefined;
    return _purchase.apply(inAppPurchase, [productID, quantity, username, paymentDiscount]);
  };
  _inAppPurchase = inAppPurchase;
} else {
  _inAppPurchase = new EventEmitter();
  Object.assign(_inAppPurchase, {
    purchaseProduct: () => {
      throw new Error('The inAppPurchase module can only be used on macOS');
    },
    canMakePayments: () => false,
    getReceiptURL: () => ''
  });
}

export default _inAppPurchase;
